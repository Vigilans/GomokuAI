#ifndef GOMOKU_HANDICRAFTS_PATTERN_H_
#define GOMOKU_HANDICRAFTS_PATTERN_H_
#include "Utilities.h"
#include <string_view>

namespace Gomoku::Handicrafts {

using std::string_view_literals::operator""sv;

enum class PatternType { 
    One, DeadTwo, LiveTwo, DeadThree, LiveThree, DeadFour, LiveFour, Five, Size
};

struct Pattern {   
    PatternType type;
    std::string_view str;
    unsigned offset; // bits相对于0位的偏移量（2bit为1单位）
    unsigned bits; // 每2bit代表一个棋位，01:己方，10:对手，11:空位，00:任意。
    
    Pattern() = default;

    constexpr Pattern(std::string_view str, PatternType type, unsigned offset = 0, unsigned bits = 0) 
        : str(str), type(type), offset(offset), bits(bits ? bits : Encode(str)) { }

    constexpr size_t length() const { return str.length() - 1; }

    constexpr bool test(unsigned bitmap) { return (bits & bitmap) == bits; }
};

constexpr Pattern PATTERN_PROTOS[] = {
    { "+xxxxx",   PatternType::Five },
    { "-_oooo_",  PatternType::LiveFour },
    { "-xoooo_",  PatternType::DeadFour },
    { "-o_ooo",   PatternType::DeadFour },
    { "-oo_oo",   PatternType::DeadFour },
    { "-_ooo_",   PatternType::LiveThree },
    { "-o_oo^",   PatternType::LiveThree },
    { "-oo_o^",   PatternType::LiveThree },
    { "-x^ooo_^", PatternType::LiveThree },
    { "-xooo__",  PatternType::DeadThree },
    { "-xoo_o_",  PatternType::DeadThree },
    { "-xo_oo_",  PatternType::DeadThree },
    { "-oo__o",   PatternType::DeadThree },
    { "-o_o_o",   PatternType::DeadThree },
    { "-x_ooo_x", PatternType::DeadThree },
    { "-__oo__",  PatternType::LiveTwo },
    { "-x^oo__",  PatternType::LiveTwo },
    { "-_o_o_",   PatternType::LiveTwo },
    { "-o__o^",   PatternType::LiveTwo },
    { "-xoo___",  PatternType::DeadTwo },
    { "-xo_o__",  PatternType::DeadTwo },
    { "-xo__o_",  PatternType::DeadTwo },
    { "-o___o",   PatternType::DeadTwo },
    { "-x_oo__x", PatternType::DeadTwo },
    { "-x_o_o_x", PatternType::DeadTwo }
};

inline const auto SURROUNDING_WEIGHTS = []() {
    Eigen::Array<
        int,
        MAX_SURROUNDING_SIZE,
        MAX_SURROUNDING_SIZE
    > W;
    W << 3, 1, 1, 2, 1, 1, 3,
         1, 4, 2, 3, 2, 4, 1,
         1, 2, 5, 4, 5, 2, 1,
         2, 3, 4, 0, 4, 3, 2,
         1, 2, 5, 4, 5, 2, 1,
         1, 4, 2, 3, 2, 4, 1,
         3, 1, 1, 2, 1, 1, 3;
    return W;
}();

template <int N, typename = std::enable_if_t<N >= 0>>
struct Reverser {
    template <size_t... I>
    constexpr static auto BuildStr(std::index_sequence<I...>) {
        return std::array<char, 2 + sizeof...(I)> { PATTERN_PROTOS[N].str[0], PATTERN_PROTOS[N].str[sizeof...(I) - I]... };
    }

    constexpr static auto BuildBits() {
        auto pattern = PATTERN_PROTOS[N];
        auto reversed_bits = 0u;
        for (auto i = 1; i <= Length; ++i) {
            reversed_bits <<= 2;
            reversed_bits |= pattern.bits & 0b11;
            pattern.bits >>= 2;
        }
        return reversed_bits;
    }

    constexpr static auto Length = PATTERN_PROTOS[N].length();

    constexpr static auto StrArray = BuildStr(std::make_index_sequence<Length>());

    constexpr static Pattern Pattern = { 
        StrArray.data(),
        PATTERN_PROTOS[N].type,
        0, BuildBits() 
    };
};


struct PatternBuilder {
    template <int N, int... I>
    constexpr static auto IndexHelper() {
        if constexpr (N == -1) {
            return std::integer_sequence<int, I...>();
        } else {
            constexpr auto original = PATTERN_PROTOS[N];
            constexpr auto reversed = Reverser<N>::Pattern;
            if constexpr (original.bits == reversed.bits) { // symmetric
                return IndexHelper<N - 1, N, I...>();
            } else { // asymmetric
                return IndexHelper<N - 1, N, -N, I...>();
            }
        }
    }

    template <int N>
    constexpr static auto Build() {
        if constexpr (N >= 0) {
            return PATTERN_PROTOS[N];
        } else {
            return Reverser<-N>::Pattern;
        }
    }

    template <int... I>
    constexpr static auto Augment(std::integer_sequence<int, I...>) {
        return std::array<Pattern, sizeof...(I)> { Build<I>()...};
    }
};

constexpr auto AUGMENTED_PATTERNS = PatternBuilder::Augment(PatternBuilder::IndexHelper<std::size(PATTERN_PROTOS) - 1>());

}

#endif // !GOMOKU_HANDICRAFTS_PATTERN_H_
