#include "Minimax.h"
#include "Pattern.h"
#include "algorithms/Heuristic.hpp"
#include <algorithm>
#include <numeric>
#include <tuple>
#include <iostream>

using namespace std;

namespace Gomoku {

using namespace Algorithms;

/* ------------------- Minimax类实现 ------------------- */

Minimax::Node::Node(Node* parent, Position position, Player player)
    : parent(parent), position(position), player(player) { }

Minimax::Minimax(
    short    c_depth,
    Position last_move,
    Player   last_player
) :
    m_root(make_unique<Node>(nullptr, last_move, last_player)),
    m_boardMap(m_evaluator.m_boardMap),
    c_constraint(Constraint::Depth) {
};

Position Minimax::getAction(Board& board) {
    m_root->value = -alphaBeta(m_root.get(), 1, -INFINITY, INFINITY);
    Eigen::MatrixXf values;
    values.setZero(15, 15);
    for (const auto& child : m_root->children) {
        auto[x, y] = child->position;
        values(y, x) = child->value;
    }
    cout << m_root->children.size() << endl;
    cout << values << endl;
    return Minimax::stepForward()->position;
}

// 更新后，原根节点由unique_ptr自动释放，其余的非子树结点也会被链式自动销毁。
Minimax::Node* Minimax::updateRoot(std::unique_ptr<Node>&& next_root) {
    m_evaluator.applyMove(next_root->position);
    m_root = std::move(next_root);
    m_root->parent = nullptr;
    return m_root.get();
}

Minimax::Node* Minimax::stepForward() {
    auto iter = max_element(m_root->children.begin(), m_root->children.end(), [](auto& lhs, auto& rhs) {
        return lhs->value < rhs->value;
    });
    return iter != m_root->children.end() ? updateRoot(std::move(*iter)) : m_root.get();
}

Minimax::Node* Minimax::stepForward(Position move) {
    auto iter = find_if(m_root->children.begin(), m_root->children.end(), [move](auto& node) {
        return node->position == move;
    });
    // 若没找到指定子结点，则新开一个结点，将其作为根节点
    if (iter == m_root->children.end()) {
        // 这个迷之hack是为了防止Python模块中出现引用Bug...
        iter = m_root->children.insert(m_root->children.end(), make_unique<Node>(nullptr, move, -m_root->player));
    }
    return updateRoot(std::move(*iter));
}

size_t Minimax::expand(Node* node, Eigen::VectorXf action_probs) {
    // 根据传入的概率将棋盘上的点降序排序
    vector<int> positions(action_probs.size());
    std::iota(positions.begin(), positions.end(), 0);
    std::sort(positions.begin(), positions.end(), [&](int lhs, int rhs) {
        return action_probs[lhs] > action_probs[rhs];
    });

    // 按序插入子结点
    for (auto position : positions) {
        if (action_probs[position] == 0.0f) {
            break; // 此后概率必定为0，均被剪枝
        }
        if (m_evaluator.board().checkMove(position)) {
            node->children.push_back(make_unique<Node>(node, position, -node->player));
        }
    }
    return node->children.size();
}

float Minimax::alphaBeta(Node* node, int depth, float alpha, float beta) {
    // Base case: 棋盘抵达结束状态或抵达深度限制
    if (m_evaluator.checkGameEnd()) {
        return CalcScore(-node->player, m_evaluator.board().m_winner);
    } else if (depth == 0) { // 以当前下棋玩家为标准返回价值评估
        return Heuristic::EvaluationValue(m_evaluator, -node->player);
    }
    //// 置换表搜索策略
    //if (m_transTable.count(m_boardMap)) {
    //    auto entry = m_transTable[m_boardMap];
    //    if (entry.depth > depth) { // 查询的记录足够深
    //        switch (entry.type) {
    //        case Entry::Alpha:
    //            
    //        case Entry::Exact:
    //            return entry.value;
    //        case Entry::Beta:
    //            
    //        }
    //    }
    //}

    // 若结点为叶结点，则新扩张一层子结点
    if (node->children.empty()) {
        // 获取相对于当前玩家而言的落子概率分布，并按概率排序扩张子结点
        auto action_probs = Heuristic::EvaluationProbs(m_evaluator, -node->player);
        //Heuristic::DecisiveFilter(m_evaluator, action_probs);
        this->expand(node, std::move(action_probs));
    }

    // Recursive Step: 按序遍历子结点，递归搜索价值
    for (const auto& child : node->children) {
        m_evaluator.applyMove(child->position);
        child->value = -alphaBeta(child.get(), depth - 1, -beta, -alpha);
        m_evaluator.revertMove();

        if (alpha < -child->value) {
            alpha = -child->value;
        }
        if (alpha >= beta) { // α截断
            return beta;
        }
    }
    return alpha;
}

void Minimax::syncWithBoard(const Board& board) {
    auto beg = std::find(board.m_moveRecord.begin(), board.m_moveRecord.end(), m_root->position);
    auto cur = (beg == board.m_moveRecord.end() ? board.m_moveRecord.begin() : ++beg);
    std::for_each(cur, board.m_moveRecord.end(), [this](auto move) { stepForward(move); });
    m_evaluator.syncWithBoard(board);
}

}