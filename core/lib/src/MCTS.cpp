#include "MCTS.h"
#include "algorithms/MonteCarlo.hpp"
#include "policies/Random.h"
#include <iostream>

using namespace std;
using namespace std::chrono;
using Eigen::VectorXf;

namespace Gomoku {

using Algorithms::Default;
using Algorithms::Stats;
using Policies::RandomPolicy;

/* ------------------- Policy类实现 ------------------- */

Policy::Policy(SelectFunc f1, ExpandFunc f2, EvalFunc f3, UpdateFunc f4, double c_puct)
    : select(f1 ? f1 : [this](auto node) { 
        return Default::Select(this, node); 
    }),
    expand(f2 ? f2 : [this](auto node, auto& board, auto probs) { 
        return Default::Expand(this, node, board, std::move(probs)); 
    }),
    simulate(f3 ? f3 : [this](auto& board) { 
        return Default::Simulate(this, board); 
    }),
    backPropogate(f4 ? f4 : [this](auto node, auto& board, auto value) { 
        return Default::BackPropogate(this, node, board, value); 
    }), 
    c_puct(c_puct) { 

}

// 若未被重写，则创建基类结点
unique_ptr<Node> Policy::createNode(Node* parent, Position pose, Player player, float value, float prob) {
    return make_unique<Node>(Node{ parent, pose, player, value, prob });
}

void Policy::prepare(Board& board) {
    m_initActs = board.m_moveRecord.size(); // 记录棋盘起始位置
}

void Policy::cleanup(Board& board) {
    board.revertMove(board.m_moveRecord.size() - m_initActs); // 保证一定重置回初始局面
}

Player Policy::applyMove(Board& board, Position move) {
    return board.applyMove(move, false); // 非终点节点无需检查是否胜利（此前已检查过了）
}

Player Policy::revertMove(Board& board, size_t count) {
    return board.revertMove(count);
}

bool Policy::checkGameEnd(Board& board) {
    return board.checkGameEnd();
}

/* ------------------- MCTS类实现 ------------------- */

// 更新后，原根节点由unique_ptr自动释放，其余的非子树结点也会被链式自动销毁。
inline Node* updateRoot(MCTS& mcts, unique_ptr<Node>&& next_node) {
    mcts.m_root = std::move(next_node);
    mcts.m_root->parent = nullptr;
    return mcts.m_root.get();
}

MCTS::MCTS(
    milliseconds c_duration,
    Position last_move,
    Player last_player,
    shared_ptr<Policy> policy
) :
    m_policy(policy ? policy : shared_ptr<Policy>(new RandomPolicy)),
    m_root(m_policy->createNode(nullptr, last_move, last_player, 0.0, 1.0)),
    m_size(1),
    m_iterations(0),
    m_duration(c_duration),
    c_constraint(Constraint::Duration) { 
    
}

MCTS::MCTS(
    size_t   c_iterations,
    Position last_move,
    Player   last_player,
    shared_ptr<Policy> policy
) :
    m_policy(policy ? policy : shared_ptr<Policy>(new RandomPolicy)),
    m_root(m_policy->createNode(nullptr, last_move, last_player, 0.0, 1.0)),
    m_size(1),
    m_iterations(c_iterations),
    m_duration(0ms),
    c_constraint(Constraint::Iterations) {

};

Position MCTS::getAction(Board& board) {
    runPlayouts(board);
    return stepForward()->position;
}

Policy::EvalResult MCTS::evalState(Board& board) {
    runPlayouts(board);
    Eigen::VectorXf child_visits;
    child_visits.setZero((int)BOARD_SIZE);
    for (auto&& node : m_root->children) {
        child_visits[node->position] = node->node_visits;
    }
    cout << Eigen::Map<const Eigen::Array<float, 15, 15, Eigen::RowMajor>>(child_visits.data()) << endl;
	child_visits = child_visits.normalized().unaryExpr([](float v) { return v ? v + 1 : v; });
    auto action_probs = Stats::TempBasedProbs(
        child_visits, board.m_moveRecord.size() < 15 ? 1 : 1e-2 
    );
    return { m_root->state_value, action_probs };
}

void MCTS::syncWithBoard(Board & board) {
    auto iter = std::find(board.m_moveRecord.begin(), board.m_moveRecord.end(), m_root->position);
    for (iter = (iter == board.m_moveRecord.end() ? board.m_moveRecord.begin() : ++iter);
        iter != board.m_moveRecord.end(); ++iter) {
        stepForward(*iter);
    }
}

// AlphaZero的论文中，对MCTS的再利用策略
// 参见https://stackoverflow.com/questions/47389700
Node* MCTS::stepForward() {
    auto iter = max_element(m_root->children.begin(), m_root->children.end(), [](auto&& lhs, auto&& rhs) {
        return lhs->node_visits < rhs->node_visits;
    });
    return iter != m_root->children.end() ? updateRoot(*this, std::move(*iter)) : m_root.get();
}

Node* MCTS::stepForward(Position next_move) {
    auto iter = find_if(m_root->children.begin(), m_root->children.end(), [next_move](auto&& node) {
        return node->position == next_move;
    });
    if (iter == m_root->children.end()) { // 这个迷之hack是为了防止Python模块中出现引用Bug...
        iter = m_root->children.emplace(
            m_root->children.end(), 
            m_policy->createNode(nullptr, next_move, -m_root->player, 0.0f, 1.0f)
        );
    }
    return updateRoot(*this, std::move(*iter));
}

void MCTS::reset() {
    auto iter = m_root->children.emplace(
        m_root->children.end(), 
        m_policy->createNode(nullptr, Position(-1), Player::White, 0.0f, 1.0f)
    );
    m_root = std::move(*iter);
    m_size = 1;
}

size_t MCTS::playout(Board& board) {
    Node* node = m_root.get();      // 裸指针用作观察指针，不对树结点拥有所有权
    while (!node->isLeaf()) {   // 检测当前结点是否所有可行手都被拓展过
        node = m_policy->select(node);  // 若当前结点已拓展完毕，则根据价值公式选出下一个探索结点
        m_policy->applyMove(board, node->position);
    }
    double node_value;
    size_t expand_size;
    if (!m_policy->checkGameEnd(board)) {  // 检查终结点游戏是否结束
        auto [state_value, action_probs] = m_policy->simulate(board); // 获取当前盘面相对于「当前应下玩家」的价值与概率分布
        expand_size = m_policy->expand(node, board, std::move(action_probs)); // 根据传入的概率向量扩展一层结点
        node_value = -state_value; // 由于node保存的是「下出变成当前局面的一手」的玩家，因此其价值应取相反数
    } else {
        expand_size = 0;
        node_value = CalcScore(node->player, board.m_winner); // 根据绝对价值(winner)获取当前局面于玩家的相对价值
    }
    m_policy->backPropogate(node, board, node_value);     
    m_policy->revertMove(board, board.m_moveRecord.size() - m_policy->m_initActs); // 重置回初始局面
    return expand_size;
}

void MCTS::runPlayouts(Board& board) {
    auto start = system_clock::now();
    this->syncWithBoard(board);
	Default::AddNoise(m_root.get());
    m_policy->prepare(board);    
    if (c_constraint == Constraint::Duration) {
        m_iterations = 0;
        for (auto end = start; end - start < m_duration; 
            end = system_clock::now(), ++m_iterations) {
            m_size += playout(board);
        }
    } else if (c_constraint == Constraint::Iterations) {
        m_duration = 0ms;
        for (auto i = 0; i < m_iterations; ++i) {
            m_size += playout(board);
        }
        m_duration = duration_cast<milliseconds>(system_clock::now() - start);
    }
    m_policy->cleanup(board);
}

}