#ifndef GOMOKU_POLICY_TRADITIONAL_H_
#define GOMOKU_POLICY_TRADITIONAL_H_
#include "../MCTS.h"
#include "../handicrafts/Evaluator.h"
#include "../algorithms/RAVE.h"
#include "../algorithms/Handicraft.h"

namespace Gomoku::Policies {

// 在该Policy下，原Board除了棋盘状态外，不下任何一子，由内部维护的棋盘进行代理。
class TraditionalPolicy : public Policy {
public:
    using Evaluator = Handicrafts::Evaluator; // 引入局面评估类
    using Default = Algorithms::Default; // 引入默认算法
    using RAVE = Algorithms::RAVE; // 引入RAVE算法
    using AMAFNode = Algorithms::AMAFNode; // 选择AMAFNode作为结点类型
    using Handicraft = Algorithms::Handicraft; // 引入人工算法

    TraditionalPolicy(double puct = 1e-4, double bias = 1e-1, bool useRave = false) :
        Policy(
            [this](auto node)                          { return RAVE::Select(this, node); },
            [this](auto node, auto& board, auto probs) { return heuristicExpand(node, m_evaluator.m_board, std::move(probs)); },
            [this](auto& board)                        { return hybridSimulate(m_evaluator.m_board); },
            [this](auto node, auto& board, auto value) { return RAVE::BackPropogate(this, node, m_evaluator.m_board, value, c_bias, c_useRave); },
            puct), c_bias(bias), c_useRave(useRave) {

    }

    virtual std::unique_ptr<Node> createNode(Node* parent, Position pose, Player player, float value, float prob) override {
        return std::unique_ptr<Node>(new AMAFNode{ parent, pose, player, value, prob });
    }

    virtual void prepare(Board& board) override {
        Policy::prepare(board);
        m_backup.syncWithBoard(board);
        m_evaluator = m_backup;
        m_cachedActs = m_initActs; // 视初始状态时已下的棋为已缓存
    }

    // 利用缓存策略下棋。
    // 此时，m_cachedActs标记成功缓存的记录区间的右界（左闭右开表示）
    virtual Player applyMove(Board& board, Position move) override {
        auto& r_board = m_evaluator.m_board;
        if (m_cachedActs == r_board.m_moveRecord.size() || r_board.m_moveRecord[m_cachedActs] != move) {
            // 没有更多缓存记录或缓存失败，回退至最大缓存状态后继续下棋
            if (r_board.m_moveRecord.size() - m_cachedActs > m_cachedActs - m_initActs) {
                // 如果缓存的太少，不如重新计算
                auto cached_record = std::move(m_evaluator.m_board.m_moveRecord);
                m_evaluator = m_backup;
                for (auto i = m_initActs; i < m_cachedActs; ++i) {
                    m_evaluator.applyMove(cached_record[i]);
                }
            } else {
                // 否则，逆向回退到原棋面
                m_evaluator.revertMove(r_board.m_moveRecord.size() - m_cachedActs);            
            }
            auto result = m_evaluator.applyMove(move); // 游戏结束时，result == m_curPlayer == Player::None
            if (m_cachedActs < r_board.m_moveRecord.size()) {
                ++m_cachedActs;
            }
            return syncBoardStatus(board), result;
        }
        // 由于policy的applyMove不检查胜利，因此返回值必为两者之一  
        return ++m_cachedActs, board.m_curPlayer = -board.m_curPlayer;
    }

    // 根据缓存策略，不直接悔到初始局面，而是悔棋至叶节点局面（最远缓存节点）+重置计数。
    virtual Player revertMove(Board& board, size_t count) override {
        auto& r_board = m_evaluator.m_board;
        r_board.revertMove(r_board.m_moveRecord.size() - m_cachedActs); // 内部棋盘只回到叶结点局面
        m_cachedActs = m_initActs; // 重置计数
        board.m_winner = Player::None; // 原棋盘回到初始状态（让Winner与CurPlayer回退）
        return board.m_curPlayer = ((m_initActs) % 2 == 1 ? Player::White : Player::Black);
    }

    // 检查完后，将原棋盘状态与内部棋盘玩家状态同步
    virtual bool checkGameEnd(Board& board) override {
        bool result = m_evaluator.m_board.checkGameEnd();
        return syncBoardStatus(board), result;
    }

    size_t heuristicExpand(Node* node, Board& board, const Eigen::VectorXf action_probs) {
        // heuristicly intialize children nodes' state_values.
        for (int i = 0; i < BOARD_SIZE; ++i) {
            if (action_probs[i] != 0.0f) {
                node->children.emplace_back(new AMAFNode {
                    node, i, -node->player, m_childrenValues[i]/*0*/, action_probs[i], m_childrenValues[i], 30
                });
            }
        }
        m_childrenValues.setZero();
        return node->children.size();
    }

    EvalResult hybridSimulate(Board& board) {
        auto init_player = board.m_curPlayer;
        Eigen::VectorXf action_probs;

        // Pre-evaluate the board using handicraft policy
        auto self_scores = Handicraft::EvalScores(m_evaluator, init_player);
        auto rival_scores = Handicraft::EvalScores(m_evaluator, -init_player);
        
        Position next_move;
        m_childrenValues = Handicraft::ScoreBasedChildrenValues(m_evaluator, board, self_scores, rival_scores);

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
            action_probs = Handicraft::ScoreBasedProbs(m_evaluator, board, self_scores, rival_scores);
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
    double c_bias;
    bool c_useRave;
    size_t m_cachedActs = 0;
    Evaluator m_evaluator;
    Evaluator m_backup; // 用于与传入的Board状态同步
    Eigen::VectorXf m_childrenValues; // 为了让Expand函数能获取到子结点价值的dirty hack...
};

}

#endif // !GOMOKU_POLICY_TRADITIONAL_H_