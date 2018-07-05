#ifndef GOMOKU_ALGORITHMS_HEURISTIC_H_
#define GOMOKU_ALGORITHMS_HEURISTIC_H_
#include "../Pattern.h"

namespace Gomoku::Algorithms::Heuristic {
   
// Return : vector of decisive pose + is pose anti-decisive
inline std::tuple<const std::vector<Position>&, bool> CheckDecisive(Evaluator& evaluator, Board& board) {
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


// logits = w * self_worthy + (1 - w) * self_anti
// action_probs = normalize(coeff_mul(logits, density_dist))
inline Eigen::VectorXf EvaluationProbs(Evaluator& ev, Player player) {
    auto logits = (6 * ev.scores(player, player) + 4 * ev.scores(player, -player)).array() / 10;
    auto weights = ev.density(player).array();
    //auto sigmoid = [](int x) { return 1 / (1 + std::exp(-x)); };
    //float temperature = 1 - sigmoid((board.m_moveRecord.size() - 30) / 20);
    return (logits * weights).cast<float>().matrix().normalized();
}


// logits = (self_worthy - rival_worthy) / scale_factor
// state_value = tanh(dot(logits, normalize(density_dist))
inline float EvaluationValue(Evaluator& ev, Player player) {
    auto logits = (ev.scores(player, player) - ev.scores(-player, -player)).cast<float>() / 10;
    auto weights = ev.density(player).cast<float>().normalized();
    return std::tanh(logits.dot(weights));
}


// 利用缓存策略下棋。
// 此时，m_cachedActs标记成功缓存的记录区间的右界（左闭右开表示）
inline Player CachedApplyMove(Board& base, Position move, Evaluator& evaluator, size_t& cached_acts) {
    auto& ref = evaluator.board();
    if (cached_acts == ref.m_moveRecord.size() || ref.m_moveRecord[cached_acts] != move) {
        // 没有更多缓存记录或缓存失败，回退至最大缓存状态后继续下棋
        if (ref.m_moveRecord.size() - cached_acts > cached_acts - 0) {
            // 如果缓存的太少，不如重新计算
            auto cached_record = std::move(ref.m_moveRecord);
            evaluator.reset();
            for (auto i = 0; i < cached_acts; ++i) {
                evaluator.applyMove(cached_record[i]);
            }
        } else {
            // 否则，逆向回退至最后成功缓存的局面
            evaluator.revertMove(ref.m_moveRecord.size() - cached_acts);
        }
        auto result = evaluator.applyMove(move); // 游戏结束时，result == m_curPlayer == Player::None
        if (cached_acts < ref.m_moveRecord.size()) {
            ++cached_acts;
        }
        // 为外部棋盘同步虚状态后，返回结果
        return base.m_curPlayer = ref.m_curPlayer, base.m_winner = ref.m_winner, result;
    }
    // 由于policy的applyMove不检查胜利，因此缓存成功时，返回值必为两者之一  
    return ++cached_acts, base.m_curPlayer = -base.m_curPlayer;
}


// 根据缓存策略，不直接悔到初始局面，而是悔棋至叶节点局面（最远缓存节点） + 重置计数。
inline Player CachedRevertMove(Board& base, size_t count, Evaluator& evaluator, size_t& cached_acts, size_t init_acts) {
    auto& ref = evaluator.board();
    ref.revertMove(ref.m_moveRecord.size() - cached_acts); // 内部棋盘只回到叶结点局面
    cached_acts = init_acts; // 重置缓存局面到与外部局面相同（也可以说是没有缓存）
    base.m_winner = Player::None; // 原棋盘回到初始状态（让Winner与CurPlayer回退）
    base.m_curPlayer = (init_acts % 2 == 1 ? Player::White : Player::Black);
    return base.m_curPlayer;
}

}

#endif // !GOMOKU_ALGORITHMS_HEURISTIC_H_