#include "handicrafts/Evaluator.h"

using namespace std;
using namespace Gomoku;
using namespace Gomoku::Handicrafts;

inline void updateByPattern(Evaluator* ev, int bitmap, int delta, Position move, Direction dir, Player perspect) {
    for (auto mask : ev->m_masks) { // start iterating mask
        auto fake_pattern = bitmap & mask;
        if (auto it = std::lower_bound(ev->m_patterns.begin(), ev->m_patterns.end(), fake_pattern,
            [](auto pattern, auto bits) { return pattern.bits == bits; }); it->bits == fake_pattern) {
            auto pattern = *it; // get src pattern
            auto view = BoardView(ev->distribution(perspect, pattern.type), move, dir); // get view & modify counts
            for (auto iter = view.rbegin(); iter != view.rend(); ++iter, pattern.bits >>= 2) {
                if (*iter != nullptr && (pattern.bits & 0b11u) == Bitmap(Player::None, perspect)) {
                    **iter += delta;
                }
            }
            break; // one player + one dir -> one pattern matching, so break from loop
        }
    }
};

void Evaluator::updateMove(Position move, Player src_player) {
    for (auto perspect_player : { Player::Black, Player::White })
    for (auto dir : { Direction::UpDown, Direction::LeftRight, Direction::UpDown, Direction::LeftRight }) {
        auto bitmap = getBitmap(move, dir, perspect_player);
        updateByPattern(this, bitmap, -1, move, dir, perspect_player); // decrease src counts
        bitmap &= Bitmap(src_player, perspect_player) << (SAMPLE_LEN / 2); // switch bitmap to dest pattern
        updateByPattern(this, bitmap, 1, move, dir, perspect_player); // increse des counts
    }
}

unsigned Evaluator::getBitmap(Position move, Direction dir, Player perspect) {
    unsigned bits = 0;
    for (auto player : { Player::White, Player::None, Player::Black })
    for (auto pState : BoardView(m_board->moveStates(player), move, dir)) {
        unsigned mask = pState ? *pState : 0;
        bits |= Bitmap(player, perspect) * mask;
        bits = (bits << 2) | (bits >> (2 * SAMPLE_LEN - 2)); // circular shift
    }
    bits &= (1 << SAMPLE_LEN) - 1; // mask unused bits;
    return bits;
}

Player Evaluator::applyMove(Position move) {
    updateMove(move, m_board->m_curPlayer);
    return m_board->applyMove(move, false);
}

Player Evaluator::revertMove(size_t count) {
    auto beg = m_board->m_moveRecord.rbegin(), end = beg + count;
    for (auto iter = beg; iter != end; ++iter) {
        updateMove(*iter, Player::None);
    }
    return m_board->revertMove(count);
}

bool Evaluator::checkGameEnd() {
    if (m_board->m_curPlayer == Player::None) {
        return true;
    }

    if (m_board->m_moveRecord.empty()) {
        return false;
    }

    for (auto player : { m_board->m_curPlayer, -m_board->m_curPlayer }) {
        if (distribution(player, PatternType::Five)[m_board->m_moveRecord.back()] != 0) {
            m_board->m_winner = player;
            m_board->m_curPlayer = Player::None;
            return true;
        }
    }

    if (m_board->moveCounts(Player::None) == 0) {
        m_board->m_winner = Player::None;
        m_board->m_curPlayer = Player::None;
        return true;
    }

    return false;
}

void Evaluator::buildPatterns() {
    const auto shift_and_insert = [this](Pattern pattern) {
        if (pattern.length == 7) {
            for (auto offset : { 1, (int)pattern.length - 2 }) {
                pattern.bits <<= offset;
                m_patterns.push_back(pattern);                
            }
        } else {
            for (auto offset = 0; offset < pattern.length; ++offset) {
                m_patterns.push_back(pattern);
                pattern.bits <<= 1;
            }
        }
    };
    for (auto pattern : PATTERN_PROTOS) {
        shift_and_insert(pattern);
        if (IsAsymmetric(pattern)) { shift_and_insert(Reverse(pattern)); }
    }
    std::sort(m_patterns.begin(), m_patterns.end(), [](auto lhs, auto rhs) { return lhs.bits < rhs.bits; });
}

void Evaluator::buildMasks() {
    // prepere special masks for 7-pieces pattern
    for (auto length : { 7 }) {
        auto mask = (1u << length) - 1;
        for (auto offset : { 1, length - 1 }) {
            m_masks.push_back(mask << offset);
        }
    }
    for (auto length : { 6, 5, 4 }) {
        auto mask = (1u << length) - 1;
        for (auto offset = 0; offset < length; ++offset) {
            m_masks.push_back(mask << offset);
        }
    }
}
