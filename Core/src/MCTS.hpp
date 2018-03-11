#pragma once
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <stack>
#include <memory>
#include "Game.hpp"

namespace Gomoku {

using namespace std;

struct Node {
    Node* parent = nullptr;
    vector<unique_ptr<Node>> children = {};
    Position position = -1;
    size_t visits = 0;
    float victory_value = 0.0;

    bool isLeaf() {
        return children.empty();
    }

    float UCB1(float expl_param) {
        return 1.0f * visits / victory_value + expl_param * sqrt(logf(parent->visits) / this->visits);
    }
};

class MCTS {
public:
    MCTS(Player player) : m_root(new Node{}), m_player(player) {
        srand(time(nullptr));
    }

    Position getNextMove(Board& board) {
        for (int i = 0; i < 100; ++i) {
            playout(board); // 完成一轮蒙特卡洛树的循环
        }
        stepForward(); // 更新蒙特卡洛树，向选出的最好手推进一步
        return m_root->position; // 返回选出的最好一手
    }

public:
    // 蒙特卡洛树的一轮循环
    void playout(Board& board) {
        Node* node = m_root.get(); // 裸指针用作观察指针，不对树结点拥有所有权
        while (!node->isLeaf()) {
            node = select(node);
            board.applyMove(node->position);
            m_moveStack.push(node->position);
        }
        float leaf_value;
        if (!board.status().end) {
            node = expand(node, board);
            leaf_value = simulate(node, board); // 根据模拟战预估当前结点价值分数
        } else {
            leaf_value = getFinalScore(m_player, board.m_winner); // 根据对局结果获取绝对价值分数
        }
        backPropogate(node, leaf_value);
        reset(board); // 重置Board到初始传入的状态
    }
    
    // AlphaZero的论文中，对MCTS的再利用策略
    // 参见https://stackoverflow.com/questions/47389700/why-does-monte-carlo-tree-search-reset-tree
    void stepForward() {
        // 选出最好的一手
        auto&& nextNode = std::move(*max_element(m_root->children.begin(), m_root->children.end(), [](auto&& lhs, auto&& rhs) {
            return lhs->visits < rhs->visits;
        }));
        // 更新蒙特卡洛树，将根节点推进至选择出的最好手的对应结点
        // 更新后，原根节点由unique_ptr自动释放，其余的非子树结点也会被链式自动销毁。
        m_root = std::move(nextNode);
        m_root->parent = nullptr;
    }

private:
    Node* select(const Node* node) const {
        return max_element(node->children.begin(), node->children.end(), [](auto&& lhs, auto&& rhs) {
            return lhs->UCB1(sqrtf(2)) < rhs->UCB1(sqrtf(2));
        })->get();
    }

    Node* expand(Node* node, Board& board) {
        // 随机选取一个元素作为新结点
        auto newPos = board.getRandomMove();
        node->children.emplace_back(new Node{ node, {}, newPos });
        return node->children.back().get(); // 返回新增的结点
    }

    float simulate(Node* node, Board& board) {
        while (true) {
            auto move = board.getRandomMove();
            auto result = board.applyMove(move);
            m_moveStack.push(move);
            if (result == Player::None) {
                return getFinalScore(m_player, board.m_winner);
            }
        }
    }

    void backPropogate(Node* node, float value) {
        do {
            node->visits += 1;
            node->victory_value += value >= 0 ? value : 0;
            node = node->parent;
            value = -value;
        } while (node != nullptr);
    }

    // TODO: 统一重置的规则（目前有栈和parent结点回溯两种方式……）
    void reset(Board& board) {
        while (!m_moveStack.empty()) {
            board.revertMove(m_moveStack.top());
            m_moveStack.pop();
        }
    }

private:
    unique_ptr<Node> m_root;
    Player m_player;
    stack<Position> m_moveStack;
};

}