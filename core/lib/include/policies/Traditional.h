#ifndef GOMOKU_POLICY_TRADITIONAL_H_
#define GOMOKU_POLICY_TRADITIONAL_H_
#include "../MCTS.h"
#include "../Pattern.h"
#include "../algorithms/MonteCarlo.hpp"
#include "../algorithms/Heuristic.hpp"

namespace Gomoku::Policies {

// 在该Policy下，原Board除了棋盘状态外，不下任何一子，由内部维护的棋盘进行代理。
class TraditionalPolicy : public Policy {
public:
    using Default = Algorithms::Default; // 引入默认算法
    using RAVE = Algorithms::RAVE; // 引入RAVE算法的Select与BackPropogate策略（对缓存友好）
    using Heuristic = Algorithms::Heuristic; // 引入人工算法

    TraditionalPolicy(double puct = C_PUCT) :
        Policy(
            [this](auto node)                          { return RAVE::Select(this, node); },
            [this](auto node, auto& board, auto probs) { return Default::Expand(this, node, m_evaluator.board(), std::move(probs), false); },
            [this](auto& board)                        { return hybridSimulate(m_evaluator.board()); },
            [this](auto node, auto& board, auto value) { return RAVE::BackPropogate<false>(this, node, m_evaluator.board(), value); },
            puct) {

    }

    virtual void prepare(Board& board) override {
        Policy::prepare(board);
        m_evaluator.syncWithBoard(board);
        m_cachedActs = m_initActs; // 视初始状态时已下的棋为已缓存
    }

    virtual Player applyMove(Board& board, Position move) override {
        return Heuristic::CachedApplyMove(board, move, m_evaluator, m_cachedActs);
    }

    virtual Player revertMove(Board& board, size_t count) override {
        return Heuristic::CachedRevertMove(board, count, m_evaluator, m_cachedActs, m_initActs);
    }

    // 检查的同时，将外部棋盘状态与内部棋盘状态同步
    virtual bool checkGameEnd(Board& board) override {
        bool result = m_evaluator.checkGameEnd();
        board.m_curPlayer = m_evaluator.board().m_curPlayer;
        board.m_winner = m_evaluator.board().m_winner;
        return result;
    }

    EvalResult hybridSimulate(Board& board) {
        auto init_player = board.m_curPlayer;   
        auto& ev = m_evaluator;
        auto action_probs = Heuristic::EvaluationProbs(m_evaluator, init_player);
        auto report = Heuristic::DecisiveFilter(m_evaluator, action_probs);
        if (report.level == report.Favour) {
            return { 1.0, action_probs };
        } else {
            // Fallback to default policy
            //Position next_move; 
            //action_probs.maxCoeff(&next_move.id);
            //board.applyMove(next_move);

            float value = Heuristic::EvaluationValue(m_evaluator, init_player);
            //Heuristic::TunedRandomRollout(board, value);
            //auto [winner, total_moves] = Heuristic::MaxEvaluatedRollout(m_evaluator, true);
            ////auto [winner, total_moves] = Default::RandomRollout(board, true);
            //auto state_value = 0.4*value + 0.6*CalcScore(init_player, winner);
            return { value, action_probs };
        }
    }

public:
    size_t m_cachedActs = 0;
    Evaluator m_evaluator;
};

}

#endif // !GOMOKU_POLICY_TRADITIONAL_H_