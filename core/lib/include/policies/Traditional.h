#ifndef GOMOKU_POLICY_TRADITIONAL_H_
#define GOMOKU_POLICY_TRADITIONAL_H_
#include "../MCTS.h"
#include "../handicrafts/Evaluator.h"
#include "../algorithms/RAVE.h"
#include "../algorithms/Handicraft.h"

namespace Gomoku::Policies {
    
class TraditionalPolicy : public Policy {
public:
    using Evaluator = Handicrafts::Evaluator; // 引入局面统计评估类
    using Default = Algorithms::Default; // 引入默认算法
    using RAVE = Algorithms::RAVE; // 引入RAVE算法
    using AMAFNode = Algorithms::AMAFNode; // 选择AMAFNode作为结点类型
    using Handicraft = Algorithms::Handicraft; // 引入人工算法

    TraditionalPolicy(double c_puct = 1e-4, double c_bias = 1e-1) :
        Policy(
            [this](auto node)                          { return RAVE::Select(this, node); },
            [this](auto node, auto& board, auto probs) { return Default::Expand(this, node, board, std::move(probs), false); }, // 不进行额外有效性检查
            [this](auto& board)                        { return defaultSimulate(board); },
            [this](auto node, auto& board, auto value) { return RAVE::BackPropogate(this, node, board, value, this->c_bias); },
            c_puct), c_bias(c_bias) {

    }

    virtual std::unique_ptr<Node> createNode(Node* parent, Position pose, Player player, float value, float prob) override {
        return std::unique_ptr<Node>(new AMAFNode{ parent, pose, player, value, prob });
    }

    virtual void prepare(Board& board) override {
        Policy::prepare(board);
        m_cachedActs = m_initActs; // 视初始状态时已下的棋为已缓存
    }

    // 利用缓存策略下棋。
    // 此时，m_cachedActs标记成功缓存的记录区间的右界（左闭右开表示）
    virtual Player applyMove(Board& board, Position move) override {
        m_evaluator.setBoard(&board);
        if (m_cachedActs == board.m_moveRecord.size() || board.m_moveRecord[m_cachedActs] != move) {
            // 没有更多缓存记录或缓存失败，回退至最大缓存状态后继续下棋
            m_evaluator.revertMove(board.m_moveRecord.size() - m_cachedActs);
            m_evaluator.applyMove(move);
        }
        // 由于policy的applyMove不检查胜利，因此返回值必为两者之一
        return (m_cachedActs++) % 2 == 1 ? Player::White : Player::Black;
    }

    // 根据缓存策略，不直接悔棋，而是重置计数。
    virtual Player revertMove(Board& board, size_t count) override {
        m_cachedActs = m_initActs;
        return board.m_curPlayer; 
    }

    virtual bool checkGameEnd(Board& board) override {
        m_evaluator.setBoard(&board);
        return m_evaluator.checkGameEnd();
    }

    //size_t heuristicExpand(Node* node, Board& board, const Eigen::VectorXf action_scores) {
    //    // Allocate memory in a pool manner
    //    size_t expand_size = action_scores.unaryExpr([](float score) { return score != 0.0f; }).count();
    //    auto pool = new AMAFNode[expand_size];
    //    node->children.reserve(expand_size);
    //    for (int i = 0; i < BOARD_SIZE; ++i) {
    //        if (action_scores[i] != 0.0f) {
    //            auto& inserted = node->children.emplace_back(pool + node->children.size());
    //            auto child = static_cast<AMAFNode*>(inserted.get());
    //            child->parent = node;
    //            child->position = i;
    //            child->player = -node->player;
    //            child->action_prob = 0;
    //            child->amaf_value = child->state_value = 1;
    //            child->amaf_visits = node->node_visits;
    //        }
    //    }
    //    return expand_size;
    //}

    EvalResult defaultSimulate(Board& board) {
        auto action_probs = Default::UniformProbs(board); // 先求出概率，因为Rollout后Board不会被还原
        //auto action_probs = Handicraft::DistBasedProbs(board);
        auto init_player = board.m_curPlayer;
        auto [winner, _] = Default::RandomRollout(board);
        return { CalcScore(init_player, winner), action_probs };
    }

    //EvalResult hybridSimulate(Board& board) {
    //    auto init_player = board.m_curPlayer;
    //    Eigen::Vector2f action_scores;

    //    // Pre-evaluate the board using handicraft policy
    //    auto [next_move, is_antiMove] = m_estimator.checkDecisive();
    //    if (next_move != Position(-1)) {
    //        action_scores = Eigen::VectorXf::Zero(BOARD_SIZE);
    //        action_scores[next_move] = 1.0;
    //        if (!is_antiMove) {
    //            return { 1.0, action_scores };
    //        }
    //    } else {
    //        action_scores = m_estimator.evalScores();
    //        action_scores.maxCoeff(&next_move.id);
    //    }
    //    
    //    // Fallback to default policy
    //    board.applyMove(next_move);
    //    auto [winner, _] = Default::RandomRollout(board);
    //    return { CalcScore(init_player, winner), action_scores };
    //}

public:
    double c_bias;
    size_t m_cachedActs = -1;
    Evaluator m_evaluator;
};

}

#endif // !GOMOKU_POLICY_TRADITIONAL_H_