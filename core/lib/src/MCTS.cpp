#include "MCTS.h"
#include "policies/Default.h"

using namespace std;
using namespace Gomoku;
using namespace Gomoku::Policies;

// 更新后，原根节点由unique_ptr自动释放，其余的非子树结点也会被链式自动销毁。
inline Node* updateRoot(MCTS& mcts, unique_ptr<Node>&& next_node) {
    mcts.m_root = std::move(next_node);
    mcts.m_root->parent = nullptr;
    return mcts.m_root.get();
}

Policy::Policy(
    SelectFunc f1, 
    ExpandFunc f2, 
    EvalFunc f3, 
    UpdateFunc f4
) : 
    select(f1 ? f1 : [this](auto node, auto c_puct) {
        return Default::select(this, node, c_puct);
    }),
    expand(f2 ? f2 : [this](auto node, auto action_probs) {
        return Default::expand(this, node, action_probs);
    }),
    simulate(f3 ? f3 : [this](auto& board) {
        return Default::simulate(this, board);
    }),
    backPropogate(f4 ? f4 : [this](auto node, auto& board, auto value) {
        return Default::backPropogate(this, node, board, value);
    }) {

}

MCTS::MCTS(
    Position last_move,
    Player last_player,
    Policy* policy,
    std::size_t c_iterations,
    double c_puct
) :
    m_root(new Node{ nullptr, last_move, last_player, 0.0, 1.0 }),   
    m_policy(policy ? policy : new Policy),
    m_size(1),
    c_iterations(c_iterations),
    c_puct(c_puct) { 

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
    vector<double> action_probs(BOARD_SIZE);
    for (auto&& node : m_root->children) {
        action_probs[node->position] = 1.0 * node->node_visits / m_root->node_visits;
    }
    return make_tuple(m_root->state_value, std::move(action_probs));
}

// AlphaZero的论文中，对MCTS的再利用策略
// 参见https://stackoverflow.com/questions/47389700
Node* MCTS::stepForward() {
    return updateRoot(*this, std::move(*max_element(m_root->children.begin(), m_root->children.end(), [](auto&& lhs, auto&& rhs) {
        return lhs->node_visits < rhs->node_visits;
    })));
}

Node* MCTS::stepForward(Position next_move) {
    return updateRoot(*this, std::move(*find_if(m_root->children.begin(), m_root->children.end(), [next_move](auto&& node) {
        return node->position == next_move;
    })));
}

size_t MCTS::playout(Board& board) {
    Node* node = m_root.get();      // 裸指针用作观察指针，不对树结点拥有所有权
    while (!node->isLeaf()) {   // 检测当前结点是否所有可行手都被拓展过
        node = m_policy->select(node, c_puct);  // 若当前结点已拓展完毕，则根据价值公式选出下一个探索结点
        board.applyMove(node->position, false); // 重要性能点：非终点节点无需检查是否胜利（此前已检查过了）
    }
    double node_value;
    size_t expand_size;
    if (!board.checkGameEnd(node->position)) {  // 检查终结点游戏是否结束  
        auto [value, action_probs] = m_policy->simulate(board);        // 根据模拟战预估当前结点对应盘面价值分数 
        expand_size = m_policy->expand(node, std::move(action_probs)); // 根据传入的概率向量扩展一层结点
        node_value = value;
    } else {
        node_value = getFinalScore(node->player, board.m_winner); // 根据对局结果获取绝对价值分数
    }
    m_policy->backPropogate(node, board, node_value);     // 回溯更新，同时重置Board到初始传入的状态
    return expand_size;
}