#ifndef GOMOKU_HANDICRAFTS_PATTERN_H_
#define GOMOKU_HANDICRAFTS_PATTERN_H_
#include "Utilities.h"

namespace Gomoku::Handicrafts {

enum class PatternType { 
    One, DeadTwo, LiveTwo, DeadThree, LiveThree, DeadFour, LiveFour, Five, Size
};

struct Pattern {
    // 每2bit代表一个棋位，01:己方，10:对手，11:空位，00:任意。
    unsigned bits;
    const PatternType type;
    const size_t length;

    constexpr Pattern(unsigned bits, PatternType type, int length = 0) 
        : bits(bits), type(type), length(length ? length : PatternLength(bits)) { }

    bool test(unsigned bitmap) { return bits & bitmap == bits; }
};

constexpr Pattern Reverse(Pattern pattern) {
    auto reversed = 0u, bits = pattern.bits;
    for (auto i = 0u; i < pattern.length; bits >>= 2, ++i) {
        reversed <<= 2;
        reversed |= bits & 0b11;
    }
    return Pattern{ reversed, pattern.type };
}

constexpr bool IsAsymmetric(Pattern pattern) {
    return pattern.bits != Reverse(pattern).bits;
}

template <typename ...Patterns>
constexpr auto BuildPatterns(Patterns... patterns) {
    return static_cast<Pattern>(patterns...);
}

constexpr Pattern PATTERN_PROTOS[] = {
    { 0b11'01'10'01'11, PatternType::LiveTwo },
    { 0b11'01'10'01'11, PatternType::LiveTwo }
};



}

#endif // !GOMOKU_HANDICRAFTS_PATTERN_H_
