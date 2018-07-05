#ifndef GOMOKU_POLICY_TRADITIONAL_H_
#define GOMOKU_POLICY_TRADITIONAL_H_
#include "../MCTS.h"
#include "../Pattern.h"
#include "../algorithms/MonteCarlo.hpp"
#include "../algorithms/Heuristic.hpp"

namespace Gomoku::Policies {

using namespace Algorithms; // 引入人工算法

// 在该Policy下，原Board除了棋盘状态外，不下任何一子，由内部维护的棋盘进行代理。
class TraditionalPolicy : public Policy {
public:
    using Default = Algorithms::Default; // 引入默认算法


    TraditionalPolicy(double puct = C_PUCT) :
        Policy(
            [this](auto node)                          { return Default::Select(this, node); },
            [this](auto node, auto& board, auto probs) { return Default::Expand(this, node, m_evaluator.board(), std::move(probs), false); },
            [this](auto& board)                        { return hybridSimulate(m_evaluator.board()); },
            [this](auto node, auto& board, auto value) { return Default::BackPropogate(this, node, m_evaluator.board(), value); },
            puct) {

    }

    virtual void prepare(Board& board) override {
        Policy::prepare(board);
        m_cachedActs = m_initActs; // 视初始状态时已下的棋为已缓存
    }

    virtual Player applyMove(Board& board, Position move) override {
        return Heuristic::CachedApplyMove(board, move, m_evaluator, m_cachedActs);
    }

    virtual Player revertMove(Board& board, size_t count) override {
        return Heuristic::CachedRevertMove(board, count, m_evaluator, m_cachedActs, m_initActs);
    }

    // 检查完后，将原棋盘状态与内部棋盘玩家状态同步
    virtual bool checkGameEnd(Board& board) override {
        bool result = m_evaluator.checkGameEnd();
        return syncBoardStatus(board), result;
    }

    EvalResult hybridSimulate(Board& board) {
        auto init_player = board.m_curPlayer;
        Eigen::VectorXf action_probs;
        
        Position next_move;

        auto [decisive_moves, is_antiMove] = Handicraft::CheckDecisive(m_evaluator, board);
        if (!decisive_moves.empty()) {
            action_probs = Eigen::VectorXf::Zero(BOARD_SIZE);
            for (auto move : decisive_moves) {
                action_probs[move] = 1.0f / decisive_moves.size();
            }
            if (!is_antiMove) {
                return { 1.0, action_probs };
            }
            next_move = decisive_moves[0];
        } else {
            action_probs = m_evaluator.scores(init_player).normalized();
            action_probs.maxCoeff(&next_move.id);
        }

        board.applyMove(next_move);

        // Fallback to default policy
        float state_value = Handicraft::ScoreBasedValue(m_evaluator, board, self_scores, rival_scores);
        //auto [winner, _] = Handicraft::RestrictedRollout(m_evaluator, board);
        //state_value = 0.5*state_value + 0.5*CalcScore(init_player, winner);
        for (int i = 0; i < 5; ++i) {
            auto [winner, total_moves] = Default::RandomRollout(board);
            auto score = CalcScore(init_player, winner);
            state_value = 0.8*state_value + 0.2*score;
            if (state_value * score < 0) { // 不同号，则回退棋盘后继续下棋
                board.revertMove(total_moves);
            } else {
                break;
            }
        }
        
        return { state_value, action_probs };
    }

private:
    // 将原棋盘与内部棋盘状态同步（尽管原棋盘一个新子也没下过）
    void syncBoardStatus(Board& board) {
        board.m_curPlayer = m_evaluator.m_board.m_curPlayer;
        board.m_winner = m_evaluator.m_board.m_winner;
    }

public:
    size_t m_cachedActs = 0;
    Evaluator m_evaluator;
    Evaluator m_backup; // 用于与传入的Board状态同步
};

}

#endif // !GOMOKU_POLICY_TRADITIONAL_H_