#ifndef GOMOKU_POLICY_RANDOM_H_
#define GOMOKU_POLICY_RANDOM_H_
#include "../MCTS.h"
#include "../algorithms/MonteCarlo.hpp"

// 每个Policy都是Algorithms名空间中静态方法的拼装
namespace Gomoku::Policies {

// 多次模拟取平均的随机策略
class RandomPolicy : public Policy {
public:
    // 引入默认算法
    using Default = Algorithms::Default; 

    RandomPolicy(double c_puct = C_PUCT, size_t c_rollouts = 5) : 
        Policy(nullptr, nullptr, [this](auto& board) { return averagedSimulate(board); }, nullptr, c_puct), 
        c_rollouts(c_rollouts) {

    }

    // 随机下棋直到游戏结束（进行多盘取平均值）
    EvalResult averagedSimulate(Board& board) {  
        auto init_player = board.m_curPlayer;
        double score = 0;

        for (int i = 0; i < c_rollouts; ++i) {
            auto [winner, total_moves] = Default::RandomRollout(board);
            // score += CalcScore(Player::Black, winner);   // 计算绝对价值，黑棋越赢越接近1，白棋越赢越接近-1
            score += CalcScore(init_player, winner);      // 计算相对于局面初始应下玩家的价值
            board.revertMove(total_moves); // 重置棋盘至传入时状态，注意赢家会重设为Player::None。
        }
        score /= c_rollouts;

        return { score, Default::UniformProbs(board) };
    }

public:
    size_t c_rollouts; // Simulate阶段随机下棋的轮数
};

}

#endif // !GOMOKU_POLICY_RANDOM_H_
