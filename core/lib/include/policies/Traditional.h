#ifndef GOMOKU_POLICY_TRADITIONAL_H_
#define GOMOKU_POLICY_TRADITIONAL_H_
#include "../MCTS.h"
#include "../algorithms/RAVE.h"

namespace Gomoku::Policies {
    
class TraditionalPolicy : public Policy {
public:
    using Default = Gomoku::Algorithms::Default; // 引入默认算法
    using RAVE = Gomoku::Algorithms::RAVE; // 引入RAVE算法
    using AMAFNode = Gomoku::Algorithms::AMAFNode; // 选择AMAFNode作为结点类型

    TraditionalPolicy(double c_puct = 0, double c_bias = 0.05) :
        Policy(
            [this](auto node)                          { return RAVE::Select(this, node); },
            [this](auto node, auto& board, auto probs) { return Default::Expand(this, node, board, std::move(probs), false); }, // 不进行额外有效性检查
            [this](auto& board)                        { return defaultSimulate(board); },
            [this](auto node, auto& board, auto value) { return RAVE::BackPropogate(this, node, board, value, this->c_bias); },
            c_puct), c_bias(c_bias) {

    }

    virtual std::unique_ptr<Node> createNode(Node* parent, Position pose, Player player, float value, float prob) {
        return std::unique_ptr<Node>(new AMAFNode{ parent, pose, player, value, prob });
    }

    EvalResult defaultSimulate(Board& board) {
        auto action_probs = Default::UniformProbs(board); // 先求出概率，因为Rollout后Board不会被还原
        auto init_player = board.m_curPlayer;
        auto [winner, _] = Default::RandomRollout(board);
        return { CalcScore(init_player, winner), action_probs };
    }

public:
    double c_bias;
};

}

#endif // !GOMOKU_POLICY_TRADITIONAL_H_