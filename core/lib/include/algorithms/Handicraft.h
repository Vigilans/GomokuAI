#ifndef GOMOKU_ALGORITHMS_HANDICRAFT_H_
#define GOMOKU_ALGORITHMS_HANDICRAFT_H_
#include "Default.h"
#include "../handicrafts/Evaluator.h"

namespace Gomoku::Algorithms {

struct Handicraft {

    using Evaluator = Handicrafts::Evaluator;
    using PType = Handicrafts::PatternType;
    using Checker = bool(*)(Evaluator& ev, Position move, Player perspect, Player curPlayer);

    // 元素值与下标相等的数组。
    static const Eigen::ArrayXi Indices;

    // 相对离棋盘平均中心的距离产生概率分布。
    static Eigen::VectorXf DistBasedProbs(Board& board) {
        auto mask = Default::BoardMask(board);
        // 获得平均中心位置
        size_t weight_k = 30;
        float mean_pose_id;
        if (board.m_moveRecord.empty()) {
            mean_pose_id = Position(WIDTH / 2, HEIGHT / 2);
        } else {
            float w = std::min(1.0 * board.m_moveRecord.size() / weight_k, 1.0);
            float init = board.m_moveRecord[0]; // 紧跟第一步黑棋位置走
            float mean = 1.0 * (Indices * (!mask).cast<int>()).sum() / (BOARD_SIZE - board.moveCounts(Player::None));
            mean_pose_id = (1 - w) * init + w * mean;
        }
        // 根据与平均中心的距离的开方计算概率分布
        Eigen::VectorXf action_probs = (Indices.cast<float>() * mask.cast<float>()).unaryExpr([&mean_pose_id](float id) {
            if (id == 0.0f) {
                return 0.0f;
            } else {
                float r_id = id - mean_pose_id;
                return 1.0f / std::sqrt(std::norm<float>({ fmod(r_id, WIDTH), r_id / WIDTH }));
            }
        });
        action_probs.normalize();
        return action_probs;
    }

    // 限定随机下棋的落子范围（一定在有棋子的地方周围）
    static std::tuple<Player, int> RestrictedRollout(Evaluator& ev, Board& board, bool revert = false) {
        auto total_moves = 0;
        auto init_player = board.m_curPlayer;
        auto ahead_move = board.m_moveRecord.back(); // 传进来之前Simulate Policy先行下了一子
        ev.updateOnePiece(1, ahead_move, -init_player);
        for (auto result = board.m_curPlayer; result != Player::None; ++total_moves) {
            Position next_move = board.getRandomMove();
            auto self = ev.distribution(result, PType::One);
            auto rival = ev.distribution(-result, PType::One);
            while (self[next_move] + rival[next_move] <= 0) {
                next_move.id = (next_move.id + 1) % BOARD_SIZE;
            }
            ev.updateOnePiece(1, next_move, result);
            result = board.applyMove(next_move);
        }
        for (int i = 1; i <= total_moves; ++i) {
            auto index = board.m_moveRecord.size() - i;
            ev.updateOnePiece(-1, board.m_moveRecord[index], index % 2 == 1 ? Player::White : Player::Black);
        }
        ev.updateOnePiece(-1, ahead_move, -init_player);
        return { board.m_winner, total_moves };
    }
    
    // Return : vector of decisive pose + is pose anti-decisive
    static std::tuple<const std::vector<Position>&, bool> CheckDecisive(Evaluator& evaluator, Board& board) {
        static std::pair<Checker, bool> flood_checking[]{
            { Check4, false }, { Check4, true },
            { CheckL3, false }, { CheckTo44, false },
            { CheckL3, true },  { CheckTo44, true },
            { CheckTo43, false }, { CheckTo43, true },
            { CheckTo33, false },
        };
        static std::vector<Position> possible_moves[2];
        static std::vector<Position> decisive_moves;
        possible_moves[0].clear(), possible_moves[1].clear();
        decisive_moves.clear();
        for (auto player : { Player::Black, Player::White }) {
            for (auto i = 0; i < BOARD_SIZE; ++i) {
                if (evaluator.distribution(player, PType::One)[i] > 0) {
                    possible_moves[player == Player::Black].push_back(i);
                }
            }
        }

        for (auto [checker, is_antiMove] : flood_checking) {
            auto player = is_antiMove ? -board.m_curPlayer : board.m_curPlayer;
            for (auto move : possible_moves[player == Player::Black]) {              
                if (checker(evaluator, move, player, board.m_curPlayer)) {
                    decisive_moves.push_back(move);
                }
            }
            if (!decisive_moves.empty()) {
                if (is_antiMove && checker == CheckL3) {
                    // 当反击对方活三时，自己的死三同样有价值
                    for (auto move : possible_moves[board.m_curPlayer == Player::Black]) {
                        if (CheckD3(evaluator, move, board.m_curPlayer, board.m_curPlayer)) {
                            decisive_moves.push_back(move);
                        }
                    }
                }
                return { decisive_moves, is_antiMove };
            }
        }
        return { decisive_moves, false };
    }

    static Eigen::VectorXf EvalScores(Evaluator& evaluator , Player perspect) {
        Eigen::VectorXf scores = Eigen::VectorXf::Zero(BOARD_SIZE);
        if (evaluator.m_board.m_moveRecord.empty()) { // 棋盘为空时指定从中心点开始
            scores[Position{ WIDTH / 2 , HEIGHT / 2 }] = 1.0f;
            return scores;
        }
        for (int i = 0; i < BOARD_SIZE; ++i) {
            if (!evaluator.m_board.moveState(Player::None, i)) {
                scores[i] = 0.0f;
                continue;
            }
            for (auto eval : { EvalTo33, EvalD3, EvalL2, EvalD2, EvalL1, EvalD1, EvalSurroundOnes }) {
                scores[i] += eval(evaluator, i, perspect, evaluator.m_board.m_curPlayer);
            }
        }
        return scores;
    }

    // action_probs = normalize(self_scores + rival_scores)
    static Eigen::VectorXf ScoreBasedProbs(Evaluator& ev, Board& board, 
        Eigen::Ref<Eigen::VectorXf> self_scores, Eigen::Ref<Eigen::VectorXf> rival_scores) {
        Eigen::VectorXf action_logits = 0.55*self_scores + 0.45*rival_scores;
        auto sigmoid = [](int x) { return 1 / (1 + std::exp(-x)); };
        float temperature = 1 - sigmoid((board.m_moveRecord.size() - 30) / 20);
        return Default::TempBasedProbs(action_logits, temperature);
    }

    // state_value = tanh(sum(self_scores - rival_scores) / scale_factor)
    static float ScoreBasedValue(Evaluator& ev, Board& board,
        Eigen::Ref<Eigen::VectorXf> self_scores, Eigen::Ref<Eigen::VectorXf> rival_scores) {
        return std::tanh((self_scores - rival_scores).sum() / 1e4);
    }

    // state_values = tanh(sum(self_scores - rival_scores) + Δscores)
    // 近似认为对手新Pattern价值很小，己方新Patttern价值很大，则Δscores ≈ rival_scores + self_scores。
    static Eigen::VectorXf ScoreBasedChildrenValues(Evaluator& ev, Board& board, 
        Eigen::Ref<Eigen::VectorXf> self_scores, Eigen::Ref<Eigen::VectorXf> rival_scores) {
        auto parent_state_score = self_scores.sum() - rival_scores.sum();
        Eigen::ArrayXf scored_values = parent_state_score + rival_scores.array() + self_scores.array();
        scored_values /= (3 * scored_values.abs().sum() / BOARD_SIZE); // 缩放scored_values，使其均值接近于±1/3，以获取有意义的tanh值。
        return scored_values.tanh();
    }

    static bool Check4(Evaluator& ev, Position move, Player perspect, Player curPlayer) {
        return ev.patternCount(move, PType::LiveFour, perspect, curPlayer) 
             + ev.patternCount(move, PType::DeadFour, perspect, curPlayer);
    }

    static bool CheckL3(Evaluator& ev, Position move, Player perspect, Player curPlayer) {
        return ev.patternCount(move, PType::LiveThree, perspect, curPlayer);
    }

    static bool CheckD3(Evaluator& ev, Position move, Player perspect, Player curPlayer) {
        return ev.patternCount(move, PType::DeadThree, perspect, curPlayer);
    }

    // 44可同时考虑两种空位（就算形成了冲四也是致命的）
    static bool CheckTo44(Evaluator& ev, Position move, Player perspect, Player curPlayer) {
        return ev.patternCount(move, PType::LiveThree, perspect, curPlayer) 
             + ev.patternCount(move, PType::DeadThree, perspect, curPlayer) >= 2;
    }

    // 43对4可考虑两种空位，对3只考虑有效位
    static bool CheckTo43(Evaluator& ev, Position move, Player perspect, Player curPlayer) {
        auto to_four = ev.patternCount(move, PType::LiveThree, perspect, curPlayer) 
                     + ev.patternCount(move, PType::DeadThree, perspect, curPlayer);
        auto to_three = ev.patternCount(move, PType::LiveTwo, perspect, perspect);
        return to_four && to_three;
    }

    // 33只考虑有效位（因为要下在有效位置才有威胁）
    static bool CheckTo33(Evaluator& ev, Position move, Player perspect, Player curPlayer) {
        return ev.patternCount(move, PType::LiveTwo, perspect, perspect) >= 2;
    }

    /* Eval 4 L3 To44 To43 均在DecisiveMove中判定，一定不会轮到它们计算分数 */

    static int EvalTo33(Evaluator& ev, Position move, Player perspect, Player curPlayer) {
        // 只有防守方才会计算33分数，此时33分数不用设太高，因为它已有两个L2得分
        return CheckTo33(ev, move, perspect, curPlayer) ? 6000 : 0;
    }

    static int EvalD3(Evaluator& ev, Position move, Player perspect, Player curPlayer) {
        return ev.patternCount(move, PType::DeadThree, perspect, curPlayer) * 2800;
    }

    static int EvalL2(Evaluator& ev, Position move, Player perspect, Player curPlayer) {
        return ev.patternCount(move, PType::LiveTwo, perspect, curPlayer) * 3000;
    }

    static int EvalD2(Evaluator& ev, Position move, Player perspect, Player curPlayer) {
        return ev.patternCount(move, PType::DeadTwo, perspect, curPlayer) * 625;
    }

    static int EvalL1(Evaluator& ev, Position move, Player perspect, Player curPlayer) {
        return ev.patternCount(move, PType::LiveOne, perspect, curPlayer) * 650;
    }

    static int EvalD1(Evaluator& ev, Position move, Player perspect, Player curPlayer) {
        return ev.patternCount(move, PType::DeadOne, perspect, curPlayer) * 200;
    }

    static int EvalSurroundOnes(Evaluator& ev, Position move, Player perspect, Player curPlayer) {
        return std::max(ev.distribution(perspect, PType::One)[move] * 20, 0);
    }
};

const Eigen::ArrayXi Handicraft::Indices = []() {
    Eigen::ArrayXi indices((int)BOARD_SIZE);
    for (int i = 0; i < indices.size(); ++i) { indices[i] = i; }
    return indices;
}();

}

#endif // !GOMOKU_ALGORITHMS_HANDICRAFT_H_