#ifndef GOMOKU_HANDICRAFTS_EVALUATOR_H_
#define GOMOKU_HANDICRAFTS_EVALUATOR_H_
#include "Pattern.h"

namespace Gomoku::Handicrafts {

class Evaluator {
public:
    Board* m_board;
    std::vector<Pattern> m_patterns;
    std::vector<unsigned> m_masks;
    std::array<char, BOARD_SIZE> m_distribution[2][(int)PatternType::Size];

public:
    Evaluator() {
        buildPatterns();
        buildMasks();
    }

    const auto& distribution(Player player, PatternType type) {
        return m_distribution[2 * (int)player - 1][(int)type];
    }

    // Evalutor应完全接管Board的职能，以避免出现bug。
    void setBoard(Board* board) { if (m_board == nullptr) { m_board = board; } }

    unsigned getBitmap(Position move, Direction dir, Player perspect);

    Player applyMove(Position move);

    Player revertMove(size_t count = 1);

    bool checkGameEnd();

private:
    void buildPatterns();

    void buildMasks();

    void updateMove(Position move, Player src_player);
};

}

#endif // !GOMOKU_HANDICRAFTS_EVALUATOR_H_
