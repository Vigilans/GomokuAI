#include "MCTS.h"
#include "policies/Random.h"

using namespace std;
using namespace Eigen;
using namespace Gomoku;
using namespace Gomoku::Algorithms;
using namespace Gomoku::Policies;

// 更新后，原根节点由unique_ptr自动释放，其余的非子树结点也会被链式自动销毁。
inline Node* updateRoot(MCTS& mcts, unique_ptr<Node>&& next_node) {
    mcts.m_root = std::move(next_node);
    mcts.m_root->parent = nullptr;
    return mcts.m_root.get();
}

// 若未被重写，则创建基类结点
unique_ptr<Node> Policy::createNode(Node* parent, Position pose, Player player, float value, float prob) {
    return make_unique<Node>(Node{ parent, pose, player, value, prob });
}

Policy::Policy(SelectFunc f1, ExpandFunc f2, EvalFunc f3, UpdateFunc f4, double c_puct)
    : select(f1 ? f1 : [this](auto node) { 
        return Default::Select(this, node); 
    }),
    expand(f2 ? f2 : [this](auto node, auto& board, auto probs) { 
        return Default::Expand(this, node, board, std::move(probs)); 
    }),
    simulate(f3 ? f3 : [this](auto& board) { 
        return Default::Simulate(board); 
    }),
    backPropogate(f4 ? f4 : [this](auto node, auto& board, auto value) { 
        return Default::BackPropogate(this, node, board, value); 
    }), 
    c_puct(c_puct) { 

}

MCTS::MCTS(
    Position last_move,
    Player last_player,
    size_t c_iterations,
    shared_ptr<Policy> policy
) :
    m_policy(policy ? policy : shared_ptr<Policy>(new RandomPolicy)),
    m_root(m_policy->createNode(nullptr, last_move, last_player, 0.0, 1.0)),
    m_size(1),
    c_iterations(c_iterations) { 
    
}

Position MCTS::getNextMove(Board& board) {
    for (int i = 0; i < c_iterations; ++i) {
        m_size += playout(board);
    }
    return stepForward()->position;
}

Policy::EvalResult MCTS::evalState(Board& board) {
    for (int i = 0; i < c_iterations; ++i) {
        m_size += playout(board);
    }
    VectorXf action_probs((int)BOARD_SIZE);
    for (auto&& node : m_root->children) {
        action_probs[node->position] = node->node_visits;
    }
    action_probs /= m_root->node_visits;
    return { m_root->state_value, action_probs };
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
    if (iter == m_root->children.end()) { // 这个迷之hack是为了防止python模块中出现引用Bug...
        iter = m_root->children.emplace(m_root->children.end(), m_policy->createNode(nullptr, next_move, -m_root->player, 0.0f, 1.0f));
    }
    return updateRoot(*this, std::move(*iter));
}

size_t MCTS::playout(Board& board) {
    m_policy->m_initActs = board.m_moveRecord.size(); // 记录棋盘起始位置
    Node* node = m_root.get();      // 裸指针用作观察指针，不对树结点拥有所有权
    while (!node->isLeaf()) {   // 检测当前结点是否所有可行手都被拓展过
        node = m_policy->select(node);  // 若当前结点已拓展完毕，则根据价值公式选出下一个探索结点
        board.applyMove(node->position, false); // 重要性能点：非终点节点无需检查是否胜利（此前已检查过了）
    }
    double node_value;
    size_t expand_size;
    if (!board.checkGameEnd()) {  // 检查终结点游戏是否结束  
        auto [state_value, action_probs] = m_policy->simulate(board); // 获取当前盘面相对于「当前应下玩家」的价值与概率分布
        expand_size = m_policy->expand(node, board, std::move(action_probs)); // 根据传入的概率向量扩展一层结点
        node_value = -state_value; // 由于node保存的是「下出变成当前局面的一手」的玩家，因此其价值应取相反数
    } else {
        _ASSERT(node->player == board.m_winner);
        expand_size = 0;
        node_value = CalcScore(node->player, board.m_winner); // 根据绝对价值(winner)获取当前局面于玩家的相对价值
    }
    m_policy->backPropogate(node, board, node_value);     // 回溯更新，同时重置Board到初始传入的状态
    return expand_size;
}

void Gomoku::MCTS::reset() {
    auto iter = m_root->children.emplace(m_root->children.end(), m_policy->createNode(nullptr, Position(-1), Player::White, 0.0f, 1.0f));
    m_root = std::move(*iter);
    m_size = 1;
}
