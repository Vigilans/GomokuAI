#ifndef GOMOKU_ALGORITHMS_DEFAULT_H_
#define GOMOKU_ALGORITHMS_DEFAULT_H_
#pragma warning(disable:4244) // 关闭收缩转换警告
#pragma warning(disable:4018) // 关闭有/无符号比较警告
#include "../MCTS.h"
#include <tuple>
#include <algorithm>

// Algorithms名空间是一组静态方法的集合，并不继承Policy。
// 为了使每个算法函数都能与Policy交互，每个函数均传入Policy指针。
// C++20的Unified call syntax或许能让这一切更加直观。
namespace Gomoku::Algorithms {

struct Default {
    
    // 仅考虑Exploration的UCB1公式。
    static double UCB1(const Node* node, double expl_param) {
        const double N = node->parent->node_visits;
        const double n_i = node->node_visits + 1;
        return expl_param * sqrt(log(N) / n_i);
    }

    // 基于选子概率权重的PUCT公式。
    static double PUCT(const Node* node, double c_puct) {
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
        return { board.m_winner, total_moves };
    }

    // 返回均匀概率分布。
    static Eigen::VectorXf UniformProbs(Board& board) {
        Eigen::VectorXf action_probs = BoardMask(board).cast<float>();
        action_probs /= board.moveCounts(Player::None);
        return action_probs;
    }

    static Node* Select(Policy* policy, const Node* node) {
        size_t max_index = 0;
        double max_score = -INFINITY;
        for (int i = 0; i < node->children.size(); ++i) {
            auto child = node->children[i].get();
            auto score = child->state_value + PUCT(child, policy->c_puct);
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
                node->children.emplace_back(policy->createNode(node, Position(i), -node->player, 0.0f, float(action_probs[i])));
            }
        }
        return node->children.size();
    }

    // 进行1局随机游戏。
    static Policy::EvalResult Simulate(Board& board) {
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

    static Eigen::VectorXf Softmax(Eigen::Ref<Eigen::VectorXf> logits) {
        Eigen::VectorXf exp_logits = logits.array().exp();
        return exp_logits / exp_logits.sum();
    }

    static Eigen::VectorXf TempBasedProbs(Eigen::Ref<Eigen::VectorXf> logits, float temperature) {
        Eigen::VectorXf temp_logits = logits.array().log() / temperature;
        return Softmax(temp_logits);
    }
};

}

#endif // !GOMOKU_ALGORITHMS_DEFAULT_H_
