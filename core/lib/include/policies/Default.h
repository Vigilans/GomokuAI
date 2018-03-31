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
        double max = 0.0;
        for (int i = 0; i < node->children.size(); ++i) {
            auto cur = node->children[i]->node_visits ? PUCT(node->children[i].get(), expl) : 1000;
            if (cur > max) {
                index = i;
                max = cur;
            }
        }
        return node->children[index].get();
    }

    // 根据传入的概率扩张结点。概率为0的Action将不被加入子结点中。
    static std::size_t expand(Policy* policy, Node* node, const std::vector<double> action_probs) {
        node->children.reserve(action_probs.size());
        for (int i = 0; i < GameConfig::BOARD_SIZE; ++i) {
            if (action_probs[i] != 0.0) {
                node->children.emplace_back(new Node{ node, i, -node->player, 0.0, action_probs[i] });
            }
        }
        return node->children.size();
    }

    // 随机下棋直到游戏结束
    static Policy::EvalResult simulate(Policy* policy, Board& board) {
        using Actions = std::vector<Position>;
        if (!policy->container.has_value()) {
            policy->container.emplace<Actions>();
        }   
        auto&& move_stack = std::any_cast<Actions>(policy->container);
        const auto node_player = board.m_curPlayer;

        for (auto result = node_player; result != Player::None; ) {
            auto move = board.getRandomMove();
            result = board.applyMove(move);
            move_stack.push_back(move);
        }

        const auto winner = board.m_winner;
        // 重置棋盘至MCTS当前根节点状态，注意赢家会被重新设为Player::None！
        while (!move_stack.empty()) {
            board.revertMove(move_stack.back());
            move_stack.pop_back();
        }

        const auto score = getFinalScore(node_player, winner);
        auto action_probs = std::vector<double>(BOARD_SIZE);
        for (int i = 0; i < BOARD_SIZE; ++i) {
            action_probs[i] = board.moveStates(Player::None)[i] ? 1.0 / board.moveCounts(Player::None) : 0.0;
        }

        return std::make_tuple(score, std::move(action_probs));
    }

    static void backPropogate(Policy* policy, Node* node, Board& board, double value) {
        for (; node != nullptr; node = node->parent, value = -value) {
            node->node_visits += 1;
            node->state_value += (value - node->state_value) / node->node_visits;
            if (node->parent != nullptr) {
                board.revertMove(node->position);
            }
        }
    }
};

}

#endif // !GOMOKU_POLICIES_H_
