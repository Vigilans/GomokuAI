#ifndef GOMOKU_ALGORITHMS_MONTECARLO_H_
#define GOMOKU_ALGORITHMS_MONTECARLO_H_
#pragma warning(disable:4244) // 关闭收缩转换警告
#pragma warning(disable:4018) // 关闭有/无符号比较警告
#include "../MCTS.h"
#include "algorithms/Statistical.hpp"
#include <algorithm>
#include <tuple>

// Algorithms名空间是一组静态方法的集合，并不继承Policy。
namespace Gomoku::Algorithms {

struct Default {
    
    // 仅考虑Exploration的UCB1公式。
    static double UCB1(const Node* node, double expl_param) {
        const double N = node->parent->node_visits;
        const double n_i = node->node_visits + 1;
        return expl_param * sqrt(log(N) / n_i);
    }

    // 基于选子概率权重的PUCB公式。
    static double PUCB(const Node* node, double c_puct) {
        const double P_i = node->action_prob;
        const double N = node->parent->node_visits;
        const double n_i = node->node_visits + 1;
        return c_puct * P_i * sqrt(N) / n_i;
    }

    // 获取当前可下点集的Mask Array。
    static auto BoardMask(const Board& board) {
        using namespace Eigen;
        return Map<const Array<bool, -1, 1>>(board.moveStates(Player::None).data(), BOARD_SIZE);
    }

    // 随机下棋直到游戏结束。棋盘会保持结束状态。
    static std::tuple<Player, int> RandomRollout(Board& board, bool revert = false) {
        auto total_moves = 0;
        for (auto result = board.m_curPlayer; result != Player::None; ++total_moves) {
            result = board.applyMove(board.getRandomMove());
        }
        auto winner = board.m_winner;
        if (revert) { 
            board.revertMove(total_moves); 
        }
        return { winner, total_moves };
    }

    // 返回均匀概率分布。
    static Eigen::VectorXf UniformProbs(Board& board) {
        Eigen::VectorXf action_probs = BoardMask(board).cast<float>();
        action_probs /= board.moveCounts(Player::None);
        
        return action_probs;
    }

    static Node* Select(Policy* policy, const Node* node) {
        size_t max_index = 0;
        double max_score = -1.0;
        for (int i = 0; i < node->children.size(); ++i) {
            auto child = node->children[i].get();
            auto score = child->state_value + PUCB(child, policy->c_puct);
            if (score > max_score) {
                max_score = score, max_index = i;
            }
        }
        return node->children[max_index].get();
    }

    // 根据传入的概率扩张结点。概率为0的Action将不被加入子结点中。
    static size_t Expand(Policy* policy, Node* node, Board& board, const Eigen::VectorXf action_probs, bool extraCheck = true) {
        node->children.reserve(action_probs.size());
        for (int i = 0; i < BOARD_SIZE; ++i) {
            // 后一个条件是额外的检查，防止不允许下的点意外添进树中（概率不为0）。
            if (action_probs[i] != 0.0 && (!extraCheck || board.checkMove(i))) {
                node->children.emplace_back(policy->createNode(node, i, -node->player, 0.0f, action_probs[i]));
            }
        }
        return node->children.size();
    }

    // 进行1局随机游戏。
    static Policy::EvalResult Simulate(Policy* policy, Board& board) {
        auto init_player = board.m_curPlayer;
        auto [winner, total_moves] = RandomRollout(board);
        board.revertMove(total_moves);
        return { CalcScore(init_player, winner), UniformProbs(board) };
    }

    static void BackPropogate(Policy* policy, Node* node, Board& board, float value) {
        for (; node != nullptr; node = node->parent, value = -value) {
            node->node_visits += 1;
            node->state_value += (value - node->state_value) / node->node_visits;
        }
    }

	static void AddNoise(Node* node, float alpha = 0.05, float epsilon = 0.25) {
		Eigen::VectorXf prior_probs;
		prior_probs.setZero(BOARD_SIZE);
		for (auto&& child : node->children) {
			prior_probs[child->position] = child->action_prob;
		}
		prior_probs *= 1 - epsilon;
		prior_probs += epsilon * Stats::DirichletNoise(prior_probs, alpha);
		for (auto&& child : node->children) {
			child->action_prob = prior_probs[child->position];
		}
	}

};

struct RAVE {

    struct AMAFNode : public Node {

        // AMAF算法的相关价值属性。
        float amaf_value = 0.0;
        size_t amaf_visits = 0;

        // 目前MSVC(Visual Studio 2017 15.6)仍未支持Extended aggregate initialization，故需手动写一个构造函数
        AMAFNode(Node* parent = nullptr, Position pose = -1, Player player = Player::None, float Q = .0f, float P = .0f, float amaf_Q = .0f, size_t amaf_N = 0)
            : Node{ parent, pose, player, Q, P }, amaf_value(amaf_Q), amaf_visits(amaf_N) { }
    };

    static double HandSelect(const AMAFNode* node, size_t eqv_param = 800) {
        double n = node->node_visits;
        double k = eqv_param;
        return sqrt(k / (3 * n + k));
    }

    static double MinMSE(const AMAFNode* node, double c_bias) {
        // Schedule that minimises Mean Squared Error
        double n = node->node_visits + 1;
        double n_ = node->amaf_visits;
        return n_ / (n + n_ + n * n_*c_bias*c_bias); // σ^2 = 1-Q^2
    }

    static double WeightedValue(const AMAFNode* node, double c_bias) {
        /*double weight = MinMSE(node, c_bias);*/
        double weight = HandSelect(node);
        return (1 - weight) * node->state_value + weight * node->amaf_value;
    }

    /*
        RAVE算法并不负责MCTS的Expand与使用Default Policy Simulate的过程，因而不提供相关函数。
        Select与BackPropogate过程需要配套使用，并且并不一定需要AMAF结点，利用其不回退的机制也是很好的。
    */

    static Node* Select(Policy* policy, const Node* node) {
        // 由于BackPropogate阶段已作过调整，只需取第一个值即可。
        return node->children[0].get();
    }

    // 反向传播更新结点价值，要求传入的Board处于游戏结束的状态。
    template <bool UseRave = true>
    static void BackPropogate(Policy* policy, Node* node, Board& board, float value, double c_bias = 0.0) {
        for (; node != nullptr; node = node->parent, value = -value) {
            size_t max_index = 0;
            double max_score = -INFINITY;
            // 计算最终得分，当UseRave为真时，更新并使用子结点的RAVE价值
            for (int i = 0; i < node->children.size(); ++i) {
                auto child_node = node->children[i].get();
                auto score = Default::PUCB(child_node, policy->c_puct);
                if constexpr (UseRave) {
                    auto rave_node = static_cast<AMAFNode*>(child_node);
                    if (board.moveState(rave_node->player, rave_node->position)) { // 要求是同一玩家下的
                        rave_node->amaf_visits += 1;
                        rave_node->amaf_value += (-value - rave_node->amaf_value) / rave_node->amaf_visits;
                    }
                    score += WeightedValue(rave_node, c_bias);
                } else {
                    score += child_node->state_value;
                }
                if (score > max_score) {
                    max_score = score, max_index = i;
                }
            }
            if (!node->children.empty()) {
                node->children[0].swap(node->children[max_index]); // 得分最大的子结点提升至容器首位
            }
            node->node_visits += 1;
            node->state_value += (value - node->state_value) / node->node_visits;
        }
    }

};

}

#endif // !GOMOKU_ALGORITHMS_DEFAULT_H_
