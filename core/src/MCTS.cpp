#include "MCTS.h"
#include <algorithm>
#include <cstdlib>
#include <ctime>

using namespace std;
using namespace Gomoku;

MCTS::MCTS(Player player, Position lastMove) : m_root(new Node{ nullptr, {}, lastMove }), m_player(player) {
    srand(time(nullptr));
    m_moveStack.reserve(width * height);
}

Position MCTS::getNextMove(Board& board) {
    for (int i = 0; i < 50000; ++i) {
        playout(board); // 完成一轮蒙特卡洛树的迭代
    }
    stepForward(); // 更新蒙特卡洛树，向选出的最好手推进一步
    return m_root->position; // 返回选出的最好一手
}

void MCTS::playout(Board& board) {
    Node* node = m_root.get();      // 裸指针用作观察指针，不对树结点拥有所有权
    while (!node->isLeaf()) {   // 检测当前结点是否所有可行手都被拓展过
        node = select(node);        // 若当前结点已拓展完毕，则根据价值公式选出下一个探索结点
        board.applyMove(node->position, false); // 重要性能点：非终点节点无需检查是否胜利（此前已检查过了）
    }
    double node_value;
    if (!board.checkGameEnd(node->position)) {  // 检查终结点游戏是否结束           
        node_value = simulate(node, board);     // 根据模拟战预估当前结点价值分数
        expand(node, board);
    } else {
        node_value = getFinalScore(m_player, board.m_winner); // 根据对局结果获取绝对价值分数
    }
    backPropogate(node, board, node_value);     // 回溯更新，同时重置Board到初始传入的状态
}
    
// AlphaZero的论文中，对MCTS的再利用策略
// 参见https://stackoverflow.com/questions/47389700/why-does-monte-carlo-tree-search-reset-tree
void MCTS::stepForward() {
    // 选出最好的一手
    auto&& nextNode = std::move(*max_element(m_root->children.begin(), m_root->children.end(), [](auto&& lhs, auto&& rhs) {
        return lhs->visits < rhs->visits;
    }));
    // 更新蒙特卡洛树，将根节点推进至选择出的最好手的对应结点
    // 更新后，原根节点由unique_ptr自动释放，其余的非子树结点也会被链式自动销毁。
    m_root = std::move(nextNode);
    m_root->parent = nullptr;
}

Node* MCTS::select(const Node* node) const {
    //return max_element(node->children.begin(), node->children.end(), [](auto&& lhs, auto&& rhs) {
    //    return lhs->UCB1(sqrt(2)) < rhs->UCB1(sqrt(2));
    //})->get();
    int index = 0;
    for (int i = 0; i < node->children.size(); ++i) {
        auto a = node->children[i]->UCB1(sqrt(2));
        auto b = node->children[index]->UCB1(sqrt(2));
        if (a > b)
            index = i;
    }
    return node->children[index].get();
}

void MCTS::expand(Node* node, const Board& board) {
    // 更新一层新结点
    node->children.reserve(board.moveCounts(Player::None));
    for (int i = 0; i < width * height; ++i) {
        if (board.moveStates(Player::None)[i]) {
            node->children.emplace_back(new Node{ node, {}, i });
        }
    }
}

double MCTS::simulate(Node* node, Board& board) {
    // 随机下棋直到游戏结束
    while (true) {
        auto move = board.getRandomMove();
        auto result = board.applyMove(move);
        m_moveStack.push_back(move);
        if (result == Player::None) { 
            Player winner = board.m_winner;
            // 重置棋盘至MCTS当前根节点状态，注意赢家会被重新设为Player::None！
            while (!m_moveStack.empty()) { 
                board.revertMove(m_moveStack.back());
                m_moveStack.pop_back();
            }
            return getFinalScore(m_player, winner);
        }
    }
}

void MCTS::backPropogate(Node* node, Board& board, double value) {
    // 根结点数据无需更新
    while (node) {
        node->visits += 1;
        node->victory_value += value >= 0 ? value : 0;
        if (node->parent != nullptr) {
            board.revertMove(node->position);
            node->ucb1 = node->UCB1(sqrtf(2));
        }
        node = node->parent;
        value = -value;
    } 
}