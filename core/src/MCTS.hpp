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
    float ucb1 = 0.0;

    bool isFull(const Board& board) const {
        return children.size() == board.moveCounts(Player::None);
    }

    float UCB1(float expl_param) {
        return 1.0f * visits / victory_value + expl_param * sqrt(logf(parent->visits + 1) / this->visits);
    }
};

class MCTS {
public:
    MCTS(Player player, Position lastMove = -1) : m_root(new Node{ nullptr, {}, lastMove }), m_player(player) {
        srand(time(nullptr));
        m_moveStack.reserve(width * height);
    }

    Position getNextMove(Board& board) {
        for (int i = 0; i < 50000; ++i) {
            playout(board); // 完成一轮蒙特卡洛树的迭代
        }
        stepForward(); // 更新蒙特卡洛树，向选出的最好手推进一步
        return m_root->position; // 返回选出的最好一手
    }

public:
    // 蒙特卡洛树的一轮迭代
    void playout(Board& board) {
        Node* node = m_root.get();      // 裸指针用作观察指针，不对树结点拥有所有权
        while (node->isFull(board)) {   // 检测当前结点是否所有可行手都被拓展过
            node = select(node);        // 若当前结点已拓展完毕，则根据价值公式选出下一个探索结点
            board.applyMove(node->position, false); // 重要性能点：非终点节点无需检查是否胜利（此前已检查过了）
        }
        float leaf_value;
        if (!board.checkGameEnd(node->position)) {  // 检查终结点游戏是否结束
            node = expand(node, board);
            leaf_value = simulate(node, board);     // 根据模拟战预估当前结点价值分数
        } else {
            leaf_value = getFinalScore(m_player, board.m_winner); // 根据对局结果获取绝对价值分数
        }
        backPropogate(node, board, leaf_value);     // 回溯更新，同时重置Board到初始传入的状态
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
            return lhs->ucb1 < rhs->ucb1;
        })->get();
    }

    Node* expand(Node* node, Board& board) {
        // 随机选取一个元素作为新结点
        auto newPos = board.getRandomMove();
        node->children.emplace_back(new Node{ node, {}, newPos });
        return node->children.back().get(); // 返回新增的结点
    }

    float simulate(Node* node, Board& board) {
        // 随机下棋直到游戏结束
        while (true) {
            auto move = board.getRandomMove();
            auto result = board.applyMove(move);
            m_moveStack.push_back(move);
            if (result == Player::None) { 
                // 重置棋盘至MCTS当前根节点状态
                while (!m_moveStack.empty()) { 
                    board.revertMove(m_moveStack.back());
                    m_moveStack.pop_back();
                }
                return getFinalScore(m_player, board.m_winner);
            }
        }
    }

    void backPropogate(Node* node, Board& board, float value) {
        do {
            board.revertMove(node->position);
            node->visits += 1;
            node->victory_value += value >= 0 ? value : 0;
            node->ucb1 = node->UCB1(sqrtf(2));
            node = node->parent;
            value = -value;
        } while (node ->parent != nullptr); // 根结点数据无需更新了
    }

private:
    unique_ptr<Node> m_root;
    Player m_player;
    std::vector<Position> m_moveStack;
};

}