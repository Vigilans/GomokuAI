#ifndef GOMOKU_HANDICRAFTS_EVALUATOR_H_
#define GOMOKU_HANDICRAFTS_EVALUATOR_H_
#include "Pattern.h"
#include <unordered_map>

namespace Gomoku::Handicrafts {

class Evaluator {
public:
    std::array<int, BOARD_SIZE> m_distribution[2][(int)PatternType::Size - 1]; // 不统计五子连珠分布
    std::unordered_map<unsigned, Pattern> m_patterns;
    std::vector<unsigned> m_masks;
    Board m_board; // 需要内部维护一个Board,以免受到外部变动的干扰。
    
public:
    Evaluator();

    auto& distribution(Player player, PatternType type) { return m_distribution[player == Player::Black][(int)type]; }

    // 同步Evaluator至传入的Board状态。
    void syncWithBoard(Board& board);

    unsigned getBitmap(Position move, Direction dir, Player perspect);

    Player applyMove(Position move);

    Player revertMove(size_t count = 1);

    // 利用Evaluator，我们可以做到快速检查游戏是否结束。
    bool checkGameEnd();

private:
    void buildPatterns();

    void buildMasks();

    void updateMove(Position move, Player src_player);

    void updatePattern(int bitmap, int delta, Position move, Direction dir, Player perspect);

    void updateOnePiece(int delta, Position move, Player src_player);
};

}

#endif // !GOMOKU_HANDICRAFTS_EVALUATOR_H_