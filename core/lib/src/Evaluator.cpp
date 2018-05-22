#include "handicrafts/Evaluator.h"

using namespace std;
using namespace Gomoku;
using namespace Gomoku::Handicrafts;

// 该方法被设为inline，不暴露给外部使用。下同。
inline void Evaluator::updatePattern(unsigned bitmap, int delta, Position move, Direction dir, Player perspect) {
    for (auto mask : Masks) {
        int fake_pattern = bitmap & mask;
        if (Patterns.count(fake_pattern)/*auto result = patterns.search(fake_pattern); result != nullptr*/) {
            auto pattern = Patterns[fake_pattern];
            /*auto pattern = *result;*/
            if (pattern.bits != fake_pattern) {
                throw pattern;
            }
            if (pattern.type == PatternType::Five) { 
                m_board.m_winner = perspect; // 因更新逻辑与棋盘状态的高耦合，这里只能设置赢家，不能改变其他状态。
            } else {
                auto view = LineView(distribution(perspect, pattern.type), move, dir);
                for (auto idx = 0; (idx = pattern.str.find('_', idx + 1)) != string_view::npos;) {
                    auto view_idx = SAMPLE_LEN - (pattern.str.length() - idx + pattern.offset);
                    *view[view_idx] += delta;
                }
            }
        }
    }
}

inline void Evaluator::updateOnePiece(int delta, Position move, Player src_player) {
    auto sgn = [](int x) { return x < 0 ? -1 : 1; }; // 0以上记为正（认为原位置没有棋）
    auto[view, lr, rd] = BlockView(distribution(src_player, PatternType::One), move);
    auto weight_block = SURROUNDING_WEIGHTS.block(lr.x(), lr.y(), rd.x() - lr.x() + 1, rd.y() - lr.y() + 1);
    view.array() += view.array().unaryExpr(sgn) * delta * weight_block; // sgn对应原位置是否有棋，delta对应下棋还是悔棋
    for (auto perspect : { Player::Black, Player::White }) {
        auto& count = distribution(perspect, PatternType::One)[move];
        switch (delta) {
        case 1: // 代表这里下了一子
            count *= -1; // 反转计数，表明这个位置已有棋子占用
            count -= 1; // 保证count一定是负数
            break;
        case -1: // 代表悔了这一子
            count += 1; // 除去之前减掉的辅助数
            count *= -1; // 反转计数，表明这里可以用来计分了
            break;
        }
    }
}

void Evaluator::updateMove(Position move, Player src_player) {
    for (auto perspect_player : { Player::Black, Player::White })
    for (auto dir : { Direction::UpDown, Direction::LeftRight, Direction::UpRight, Direction::DownRight }) {
        auto bits = getBitmap(move, dir, perspect_player);
        updatePattern(bits, -1, move, dir, perspect_player); // find original pattern and decrease count      
        bits = bits & ~(0b11 << 2*(SAMPLE_LEN/2)) | (Bitmap(src_player, perspect_player) << 2*(SAMPLE_LEN/2)); // set bitmap with move
        updatePattern(bits, 1, move, dir, perspect_player); // find applied pattern and increase count
    }
    if (src_player != Player::None) {
        updateOnePiece(1, move, m_board.m_curPlayer);
    } else {
        updateOnePiece(-1, move, m_board.m_moveRecord.size() % 2 == 1 ? Player::Black : Player::White);
    }
}

void Evaluator::syncWithBoard(Board& board) {
    for (int i = 0; i < board.m_moveRecord.size(); ++i) {
        if (i < m_board.m_moveRecord.size()) {
            if (m_board.m_moveRecord[i] == board.m_moveRecord[i]) {
                continue;
            } else {
                revertMove(m_board.m_moveRecord.size() - i);
            }
        }
        applyMove(board.m_moveRecord[i]);
    }
}

unsigned Evaluator::getBitmap(Position move, Direction dir, Player perspect) {
    unsigned bits = 0;
    for (auto player : { Player::White, Player::None, Player::Black })
    for (auto pState : LineView(m_board.moveStates(player), move, dir)) {
        bits = (bits << 2) | (bits >> (2*SAMPLE_LEN - 2)); // circular shift
        bits |= pState ? ((*pState) ? Bitmap(player, perspect) : 0b00) : 0b10; 
    }
    bits &= (1 << 2*SAMPLE_LEN) - 1; // mask unused bits;
    return bits;
}

void Evaluator::reset() {
    initDistributions();
    m_board.reset();
}

Player Evaluator::applyMove(Position move) {
    if (m_board.m_curPlayer != Player::None && m_board.checkMove(move)) {
        updateMove(move, m_board.m_curPlayer);
        return m_board.applyMove(move, false); // 赢家在updateMove中被检测
    }
    return m_board.m_curPlayer;
}

Player Evaluator::revertMove(size_t count) {
    for (auto i = 0; i < count && !m_board.m_moveRecord.empty(); ++i) {
        updateMove(m_board.m_moveRecord.back(), Player::None);
        m_board.revertMove();
    }
    return m_board.m_curPlayer;
}

bool Evaluator::checkGameEnd() {
    if (m_board.m_curPlayer == Player::None) {
        return true;
    }

    if (m_board.moveCounts(Player::None) == 0) {
        m_board.m_winner = Player::None;
        m_board.m_curPlayer = Player::None;
        return true;
    }

    return false;
}

void Evaluator::initDistributions() {
    for (auto& medium : m_distributions)
    for (auto& distribution : medium) {
        distribution.fill(0);
    }
}

Evaluator::Evaluator() {
    initDistributions();
}

#ifdef AUGMENTED_PATTERNS
inline Pattern RuntimeReverse(Pattern pattern) {
    static unordered_map<unsigned, string> reversed_strings;

    if (!reversed_strings.count(pattern.bits)) {
        string reversed_str(pattern.str.data(), pattern.str.length());
        std::reverse(reversed_str.begin() + 1, reversed_str.end());
        reversed_strings[pattern.bits] = std::move(reversed_str);
    } 
    pattern.str = reversed_strings[pattern.bits];

    auto reversed_bits = 0u;
    for (auto i = 1; i <= pattern.length(); ++i) {
        reversed_bits <<= 2;
        reversed_bits |= pattern.bits & 0b11;
        pattern.bits >>= 2;
    }

    pattern.bits = reversed_bits;
    return pattern;
}

#define REVERSE_PROC(x, y)\
if (x.bits != RuntimeReverse(x).bits) {\
    shift_and_insert(RuntimeReverse(x), y);\
}
#else
#define REVERSE_PROC(x) (void*)(0)
#endif

inline void shift_and_insert(Pattern pattern, decltype(Evaluator::Patterns)& patterns) {
    // PATTERN需要覆盖整个SAMPLE
    for (auto offset = 0; offset < SAMPLE_LEN - pattern.length() + 1; ++offset) {
        patterns[pattern.bits] = pattern;
        pattern.bits <<= 2;
        pattern.offset += 1;
    }
};

std::unordered_map<unsigned, Pattern, BitmapHasher> Evaluator::Patterns = []() {
    using Gomoku::Handicrafts::Pattern;
    decltype(Patterns) patterns;
    patterns.reserve(1 << (HASH_LEN - 1));

    for (auto pattern : AUGMENTED_PATTERNS) {
        shift_and_insert(pattern, patterns);
        REVERSE_PROC(pattern, patterns);
    }
    return patterns;
}();

inline auto InitPatterns() {
    vector<Pattern> ps;
    auto shift_and_insert = [](Pattern pattern, decltype(ps)& patterns) {
        // PATTERN需要覆盖整个SAMPLE
        for (auto offset = 0; offset < SAMPLE_LEN - pattern.length() + 1; ++offset) {
            patterns.push_back(pattern);
            pattern.bits <<= 2;
            pattern.offset += 1;
        }
    };
    for (auto pattern : AUGMENTED_PATTERNS) {
        shift_and_insert(pattern, ps);
        REVERSE_PROC(pattern, ps);
    }
    std::sort(ps.begin(), ps.end(), [](auto& lhs, auto& rhs) { return lhs.bits < rhs.bits; });
    return MSTree<unsigned, Pattern>(ps.begin(), ps.end(), [](auto& p) { return p.bits; });
}

MSTree<unsigned, Pattern> Evaluator::patterns = InitPatterns();

std::vector<unsigned> Evaluator::Masks = []() {
    decltype(Masks) masks;
    for (auto length : { 7, 6, 5 }) {
        auto mask = (1u << 2 * length) - 1;
        mask <<= 2 * (SAMPLE_LEN / 2 - length + 1); // 对齐至中间
        for (auto offset = 0; offset < length; ++offset) {
            masks.push_back(mask);
            mask <<= 2;
        }
    }
    return masks;
}();
