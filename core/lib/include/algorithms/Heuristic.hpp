#ifndef GOMOKU_ALGORITHMS_HEURISTIC_H_
#define GOMOKU_ALGORITHMS_HEURISTIC_H_
#include "../Pattern.h"
#include "Statistical.hpp"
#include <deque>
#include <iostream>
#include <iostream>

namespace Gomoku::Algorithms {
   
struct Heuristic {

    // w_self_worthy = self_worthy * normalize(self_density)
    // w_rival_anti = rival_anti * normalize(rival_density)
    // action_probs = normalize(w * w_self_worthy + (1-w) * w_rival_anti)
    static Eigen::VectorXf EvaluationProbs(Evaluator& ev, Player player) {
        using namespace Eigen;
        Position center(WIDTH / 2, HEIGHT / 2), second;
        VectorXf action_probs = VectorXf::Zero(BOARD_SIZE);
        Map<MatrixXf> square_probs(action_probs.data(), HEIGHT, WIDTH);
        // 五子棋规则的指定开局设计，也能在前期协助约束
        switch (ev.board().m_moveRecord.size()) {
        case 0: // 第一子应落在天元
            action_probs[center] = 1.0f;
            break;
        case 1: // 第二子落在天元周围1格内（直指/斜指）
            square_probs.block<3, 3>(center.y() - 1, center.x() - 1).fill(1.0f);
            action_probs[center] = 0.0f; // 除去中心点概率
            break;
        case 2: // 第三子落在天元周围2格内（共26种）
            second = ev.board().m_moveRecord[1];
            square_probs.block<5, 5>(center.y() - 2, center.x() - 2).fill(1.0f);
            action_probs[center] = 0.0f; // 除去第一子概率
            action_probs[second] = 0.0f; // 除去第二子概率
            for (auto sgn : { 1, -1 })   // 除去彗星/流星/游星开局
            for (auto dir : { Direction::LeftDiag, Direction::RightDiag }) {
                Vector2i a(second.y() - center.y(), second.x() - center.x());
                Vector2i b(sgn * (*dir).second, sgn * (*dir).first);
                if (a.dot(b) > 0) { // 夹角小于90°的是长星/疏星
                    continue;
                }
                action_probs[Shift(center, sgn*2, dir)] = 0.0f;
            }
            break;
        default: // 默认情况按照估值函数返回
            VectorXf self_worthy = ev.scores(player, player).cast<float>().cwiseProduct(DensityWeight(ev, player));
            VectorXf rival_anti = ev.scores(-player, player).cast<float>().cwiseProduct(DensityWeight(ev, -player));
            action_probs = 0.6 * self_worthy + 0.4 * rival_anti;
        }
        action_probs.normalize();
        return action_probs;
    }

    // ws_self_worthy = dot(self_worthy, normalize(self_density))
    // ws_rival_worthy = dot(rival_worthy, normalize(rival_density))
    // state_value = tanh((ws_self_worthy - ws_rival_worthy) / scale_factor)
    static float EvaluationValue(Evaluator& ev, Player player, float scale = 3000.0f, bool isMinimax = false) {
        //auto self_worthy  = ev.scores(player, player).cast<float>().dot(DensityWeight(ev, player));
        //auto rival_worthy = ev.scores(-player, -player).cast<float>().dot(DensityWeight(ev, -player));
		if (isMinimax) return (ev.patternScores(player).sum() - ev.patternScores(-player).sum())/ scale;
        return std::tanh((500 + ev.patternScores(player).sum() - ev.patternScores(-player).sum()) / scale);
    }

    // weight = normalize(w/n * 1.5n/(0.5+n)) = normalize(3w/(1+2n))
    static Eigen::VectorXf DensityWeight(Evaluator& ev, Player player) {
        const auto filter = [](int x) { return std::max(x, 0); };
        const auto [counts, weights] = ev.density(player);
        auto N = counts.unaryExpr(filter).cast<float>();
        auto W = weights.unaryExpr(filter).cast<float>();
        return ((3 * W) / (1 + 2 * N)).matrix().normalized();
    }

    template <typename Func>
    static std::tuple<Player, int> EvaluatedRollout(Evaluator& ev, Func probs_to_move, bool revert = false) {
        auto total_moves = 0;
        auto& board = ev.board();  
        for (; !ev.checkGameEnd(); ++total_moves) {
            auto action_probs = EvaluationProbs(ev, board.m_curPlayer);
            ev.applyMove(probs_to_move(board, action_probs));
        }
        auto winner = board.m_winner;
        if (revert) {
            ev.revertMove(total_moves);
        }
        return { winner, total_moves };
    }

    // 每一轮选取对当前玩家概率最大的点下棋，直到游戏结束。
    static std::tuple<Player, int> MaxEvaluatedRollout(Evaluator& ev, bool revert = false) {
        return EvaluatedRollout(ev, [](const Board& board, Eigen::Ref<Eigen::VectorXf> action_probs) {
            Position next_move;
            action_probs.maxCoeff(&next_move.id);
            return next_move;
        }, revert);
    }

    // 每一轮根据当前玩家的概率分布随机选点下棋，直到游戏结束。
    static std::tuple<Player, int> RandomEvaluatedRollout(Evaluator& ev, bool revert = false) {
        return EvaluatedRollout(ev, [](const Board& board, Eigen::Ref<Eigen::VectorXf> action_probs) {
            float temperature = 1 - Stats::Sigmoid((board.m_moveRecord.size() - 40) / 20.0f);
            return board.getRandomMove(action_probs);
        }, revert);
    }

    // 优先度：+4 > -4 > +L3 == +To44 > -L3 == -To44 >= +To43 > -To43 > +To33 > -To33
    static auto DecisiveFilter(Evaluator& ev, Eigen::Ref<Eigen::VectorXf> probs) {
        // 数据准备
        static std::deque<std::tuple<int, Player>> candidates;
        struct { enum { Anti, Favour, None } level = None; } report;
        enum State { _4, L3, To44, To43, To33, End } state = _4;
        auto cur_player = ev.board().m_curPlayer;
        bool is_antiMove = false;
        constexpr std::tuple<State, bool> AutomataTable[2][6] = {
        /* anti/state   _4       L3         To44      To43        To33        End   */
        /*    0 */ { {_4, 1}, {To44, 0}, { L3, 1 }, {To43, 1}, {To33, 1}, { End, 0 } },
        /*    1 */ { {L3, 0}, {To44, 1}, {To43, 0}, {To33, 0}, { End, 0}, { End, 1 } }
        };
        // 自动机范式编程
        while (state != State::End) {
            // 准备待检测模式
            switch (auto player = is_antiMove ? -cur_player : cur_player; state) {
                case _4: 
                    candidates.push_back({ Pattern::LiveFour, player });
                    candidates.push_back({ Pattern::DeadFour, player });
                    break;
                case L3:
                    candidates.push_back({ Pattern::LiveThree, player });
                    break;
                case To44: case To43: case To33:
                    // To33 - state值与Compound各Type对应，Pattern::Size为偏移值。
                    candidates.push_back({ Pattern::Size + (To33 - state), player });
                    break;
            }
            // 筛选至寻找到有计数的模式
            while (!candidates.empty()) {
                auto [pattern, player] = candidates.front();
                auto count = 0;
                if (pattern < Pattern::Size) {
                    count = ev.m_patternDist.back()[pattern].get(player);
                } else {
                    count = ev.m_compoundDist.back()[pattern - Pattern::Size].get(player);
                }
                if (count != 0) { // 成功找到一个有计数的模式
                    // 额外反击棋型判断
                    if (is_antiMove) {
                        // 当反击对方非四连棋型时，己方眠三同样有价值
                        if (state != State::_4) {
                            candidates.push_back({ Pattern::DeadThree, -player });
                        } 
                        // 当反击对方双三棋型时，己方活二同样有价值
                        if (state == State::To33) {
                            candidates.push_back({ Pattern::LiveThree, -player });
                        }
                    }
                    break; // 直接跳出，保留后面的候选者
                }
                candidates.pop_front();
            }
            // 如果仍有候选者，则寻找成功
            if (int i = -1; !candidates.empty()) {
                report.level = is_antiMove ? report.Anti : report.Favour;
                // 将所有非Decisive点概率全部Mask为0
                probs = probs.unaryExpr([&](float prob) { // 这个较别扭的写法是为了vectorize的性能……
                    return ++i, std::any_of(candidates.begin(), candidates.end(), [&](auto candidate) {
                        auto [pattern, player] = candidate;
                        if (pattern < Pattern::Size) {
                            return ev.m_patternDist[i][pattern].get(player, cur_player);
                        } else {
                            return ev.m_compoundDist[i][pattern % Pattern::Size].get(player, cur_player);
                        }
                    });
                }).select(probs, Eigen::VectorXf::Zero(BOARD_SIZE)); 
                probs.normalize(); // 重新标准化概率
                candidates.clear(); // 清空候选队列
                state = State::End; // 状态直接跳转到结束
            } else { // 否则，状态沿正常路线转移
                std::tie(state, is_antiMove) = AutomataTable[is_antiMove][state]; // 解构赋值
            }
        }
        return report;
    }

    // 利用缓存策略下棋。
    // 此时，m_cachedActs标记成功缓存的记录区间的右界（左闭右开表示）
    static Player CachedApplyMove(Board& base, Position move, Evaluator& evaluator, size_t& cached_acts) {
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
        assert(cached_acts < ref.m_moveRecord.size());
        return ++cached_acts, base.m_curPlayer = -base.m_curPlayer;
    }

    // 根据缓存策略，不直接悔到初始局面，而是悔棋至叶节点局面（最远缓存节点） + 重置计数。
    static Player CachedRevertMove(Board& base, size_t count, Evaluator& evaluator, size_t& cached_acts, size_t init_acts) {
        auto& ref = evaluator.board();
        ref.revertMove(ref.m_moveRecord.size() - cached_acts); // 内部棋盘只回到叶结点局面
        cached_acts = init_acts; // 重置缓存局面到与外部局面相同（也可以说是没有缓存）
        base.m_winner = Player::None; // 原棋盘回到初始状态（让Winner与CurPlayer回退）
        base.m_curPlayer = (init_acts % 2 == 1 ? Player::White : Player::Black);
        return base.m_curPlayer;
    }
};

}

#endif // !GOMOKU_ALGORITHMS_HEURISTIC_H_