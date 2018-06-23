#ifndef GOMOKU_PATTERN_MATCHING_H_
#define GOMOKU_PATTERN_MATCHING_H_
#include "Game.h"
#include <utility>
#include <string_view>

namespace Gomoku {

inline namespace Config {
// 模式匹配模块的相关配置
enum PatternConfig {
    MAX_PATTERN_LEN = 7,
    MAX_SURROUNDING_SIZE = 2*3 + 1,
    TARGET_LEN = 2 * MAX_PATTERN_LEN - 1
};
}

enum class Direction : char {
    Horizontal, Vertical, LeftDiag, RightDiag
};

// 将方向枚举解包成具体的(Δx, Δy)值。
constexpr std::pair<int, int> operator*(Direction direction) {
    switch (direction) {
        case Direction::Horizontal: return { 1, 0 };
        case Direction::Vertical:   return { 0, 1 };
        case Direction::LeftDiag:   return { 1, 1 };
        case Direction::RightDiag:  return { -1, 1 };
        default: return { 0, 0 };
    }
}


struct Pattern {
    enum Type {
        One, DeadOne, LiveOne, DeadTwo, LiveTwo, DeadThree, LiveThree, DeadFour, LiveFour, Five, Size
    };

    std::string str;
    Player belonging;
    Type type;
    int score;

    Pattern(std::string_view proto, Type type, int score);
    Pattern(const Pattern&) = default;
    Pattern(Pattern&&) = default;
};


class PatternSearch {
public:
    struct generator {
        generator();

        const generator& operator++();
        bool operator!=(const generator&) const;
        std::tuple<const Pattern&, int> operator*();

        const generator& begin();
        const generator& end();
    };

    PatternSearch(std::initializer_list<Pattern> protos);
    
    generator matches(std::string_view str);

public:

};


class BoardMap {
public:
    BoardMap(Board* board = nullptr);

    std::string_view lineView(Position pose, Direction direction);

    Player applyMove(Position move);

    Player revertMove(size_t count = 1);

    void reset();

private:
    std::tuple<int, int> parseIndex(Position pose, Direction direction);

public:
    std::unique_ptr<Board> m_board;
    std::array<std::string, 3*(WIDTH + HEIGHT) - 2> m_lineMap;
    unsigned long long m_hash;
};

class Evaluator {
public:
    // 在不同的Evaluator间共享的变量
    static PatternSearch Patterns;
    BoardMap m_boardMap;
    std::array<int, BOARD_SIZE> m_distributions[2][Pattern::Type::Size - 1]; // 不统计五子连珠分布

public:
    Evaluator();

    auto& distribution(Player player, Pattern::Type type) { return m_distributions[player == Player::Black][(int)type]; }
    auto& board() { return *m_boardMap.m_board; }

    int patternCount(Position move, Pattern::Type type, Player perspect, Player curPlayer);

    Player applyMove(Position move);

    Player revertMove(size_t count = 1);

    // 利用Evaluator，我们可以做到快速检查游戏是否结束。
    bool checkGameEnd();

    // 同步Evaluator至传入的Board状态。
    void syncWithBoard(const Board& board);

    void reset();

    void updateOnePiece(int delta, Position move, Player src_player);

private:
    void initDistributions();

    void updateMove(Position move, Player src_player);

    void updatePattern(std::string_view target, int delta, Position move, Direction dir);
};

}

namespace std {

template<typename Enum = std::enable_if_t<false>>
constexpr size_t size() { return 0; }  /* will never get here */ 

template<>
constexpr size_t size<Gomoku::Direction>() { return 4; }

template<>
constexpr size_t size<Gomoku::Pattern::Type>() { return Gomoku::Pattern::Size; }

}

#endif // !GOMOKU_PATTERN_MATCHING_H_
