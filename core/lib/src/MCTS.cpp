#include "MCTS.h"
#include "policies/Default.h"

using namespace std;
using namespace Gomoku;
using namespace Gomoku::Policies;

Policy::Policy(SelectFunc f1, ExpandFunc f2, EvaluateFunc f3, UpdateFunc f4) :
    select       (f1 ? f1 : SelectFunc([this](auto node, auto c_puct) { return Default::select(this, node, c_puct); })),
    expand       (f2 ? f2 : ExpandFunc([this](auto node, auto action_probs) { return Default::expand(this, node, action_probs); })),
    simulate     (f3 ? f3 : EvaluateFunc([this](auto& board) { return Default::simulate(this, board); })),
    backPropogate(f4 ? f4 : UpdateFunc([this](auto node, auto& board, auto value) { return Default::backPropogate(this, node, board, value); })) {

}

MCTS::MCTS(
    Position last_move,
    Player last_player,
    Policy* policy,
    std::size_t c_iterations,
    double c_puct
) :
    m_root(new Node{ nullptr, {}, last_move, last_player, 0, 0.0, 1.0 }),   
    m_policy(policy ? policy : new Policy),
    m_size(1),
    c_iterations(c_iterations),
    c_puct(c_puct) {
    
}

Position MCTS::getNextMove(Board& board) {
    for (int i = 0; i < c_iterations; ++i) {
        playout(board); // 完成一轮蒙特卡洛树的迭代
    }
    return stepForward()->position; // 更新蒙特卡洛树，向选出的最好手推进一步并将其返回
}

void MCTS::playout(Board& board) {
    Node* node = m_root.get();      // 裸指针用作观察指针，不对树结点拥有所有权
    while (!node->isLeaf()) {   // 检测当前结点是否所有可行手都被拓展过
        node = m_policy->select(node, c_puct);        // 若当前结点已拓展完毕，则根据价值公式选出下一个探索结点
        board.applyMove(node->position, false); // 重要性能点：非终点节点无需检查是否胜利（此前已检查过了）
    }
    double node_value;
    if (!board.checkGameEnd(node->position)) {  // 检查终结点游戏是否结束  
        auto [value, action_probs] = m_policy->simulate(board);    // 根据模拟战预估当前结点价值分数 
        m_size += m_policy->expand(node, std::move(action_probs)); // 根据传入的概率向量扩展一层结点
        node_value = value;
    } else {
        node_value = getFinalScore(node->player, board.m_winner); // 根据对局结果获取绝对价值分数
    }
    m_policy->backPropogate(node, board, node_value);     // 回溯更新，同时重置Board到初始传入的状态
}

// AlphaZero的论文中，对MCTS的再利用策略
// 参见https://stackoverflow.com/questions/47389700/why-does-monte-carlo-tree-search-reset-tree
Node* MCTS::stepForward() {
    // 选出最好的一手
    auto&& nextNode = std::move(*max_element(m_root->children.begin(), m_root->children.end(), [](auto&& lhs, auto&& rhs) {
        return lhs->node_visits < rhs->node_visits;
    }));
    // 更新蒙特卡洛树，将根节点推进至选择出的最好手的对应结点
    // 更新后，原根节点由unique_ptr自动释放，其余的非子树结点也会被链式自动销毁。
    m_root = std::move(nextNode);
    m_root->parent = nullptr;
    return m_root.get();
}

