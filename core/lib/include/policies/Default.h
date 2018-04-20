#ifndef GOMOKU_POLICIES_DEFAULT_H_
#define GOMOKU_POLICIES_DEFAULT_H_
#pragma warning(disable:4244) // 关闭收缩转换警告
#include "MCTS.h"
#include <cstdlib>
#include <tuple>
#include <algorithm>

namespace Gomoku::Policies {

// Default是一组静态方法的集合，并不继承Policy。
// 为了使Default能使用到Policy的Container，每个函数均传入Policy指针。
// C++20的Unified call syntax或许能让这一切更加直观。
struct Default {
    
    // 仅考虑当前场面价值的UCB1公式。
    static double UCB1(const Node* node, double expl_param) {
        const double Q_i = node->state_value;
        const double N = node->parent->node_visits;
        const double n_i = node->node_visits + 1;
        return Q_i + expl_param * sqrt(log(N) / n_i);
    }

    // 基于概率的多项式UCT公式。
    static double PUCT(const Node* node, double c_puct) {
        const double Q_i = node->state_value;
        const double P_i = node->action_prob;
        const double N = node->parent->node_visits;
        const double n_i = node->node_visits + 1;
        return Q_i + c_puct * P_i * sqrt(N) / n_i;
    }

    static Node* select(Policy* policy, const Node* node, double expl) {
        //return max_element(node->children.begin(), node->children.end(), [](auto&& lhs, auto&& rhs) {
        //    return lhs->UCB1(sqrt(2)) < rhs->UCB1(sqrt(2));
        //})->get();
        int index = 0;
        double max = PUCT(node->children[0].get(), expl);
        for (int i = 1; i < node->children.size(); ++i) {
            //auto cur = node->children[i]->node_visits ? PUCT(node->children[i].get(), expl) : 1000;
            auto cur = PUCT(node->children[i].get(), expl);
            if (cur > max) {
                index = i;
                max = cur;
            }
        }
        return node->children[index].get();
    }

    // 根据传入的概率扩张结点。概率为0的Action将不被加入子结点中。
    static size_t expand(Policy* policy, Node* node, Board& board, const Eigen::VectorXf action_probs) {
        node->children.reserve(action_probs.size());
        for (int i = 0; i < GameConfig::BOARD_SIZE; ++i) {
            // 后一个条件是额外的检查，防止不允许下的点意外添进树中。
            // 一般情况下会被短路，不影响性能。
            if (action_probs[i] != 0.0 && board.checkMove(i)) {
                node->children.emplace_back(policy->createNode(node, Position(i), -node->player, 0.0f, float(action_probs[i])));
            }
        }
        return node->children.size();
    }

    // 随机下棋直到游戏结束
    static Policy::EvalResult simulate(Policy* policy, Board& board) {
        auto init_player = board.m_curPlayer;
        double score = 0;
        size_t rollout_count = 5;

        for (int i = 0; i < rollout_count; ++i) {
            auto total_moves = 0;

            for (auto result = board.m_curPlayer; result != Player::None; ++total_moves) {
                auto move = board.getRandomMove();
                result = board.applyMove(move);
            }

            //score += calcScore(Player::Black, board.m_winner); // 计算绝对价值，黑棋越赢越接近1，白棋越赢越接近-1
            score += calcScore(init_player, board.m_winner);     // 计算相对于局面初始应下玩家的价值
  
            board.revertMove(total_moves); // 重置棋盘至传入时状态，注意赢家会重设为Player::None。
        }
        
        score /= rollout_count;

        using namespace Eigen;
        VectorXf action_probs = Map<Matrix<bool, -1, 1>>(board.moveStates(Player::None).data(), BOARD_SIZE).cast<float>();
        action_probs /= board.moveCounts(Player::None);

        return { score, action_probs };
    }

    static void backPropogate(Policy* policy, Node* node, Board& board, double value) {
        for (; node != nullptr; node = node->parent, value = -value) {
            node->node_visits += 1;
            node->state_value += (value - node->state_value) / node->node_visits;
            if (node->parent != nullptr) {
                board.revertMove();
            }
        }
    }
};

}

#endif // !GOMOKU_POLICIES_H_
