#ifndef GOMOKU_HANDICRAFTS_UTILITIES_H_
#define GOMOKU_HANDICRAFTS_UTILITIES_H_
#include "../Game.h"

namespace Gomoku::Handicrafts {

enum {
    SAMPLE_LEN = 2 * MAX_RENJU + 1
};

enum class Direction : char {
    UpDown, LeftRight, UpRight, DownRight
};

constexpr std::pair<int, int> DeltaXY(Direction direction) {
    switch (direction) {
    case Direction::UpDown:    return { 0, 1 };
    case Direction::LeftRight: return { 1, 0 };
    case Direction::UpRight:   return { 1, 1 };
    case Direction::DownRight: return { 1, -1 };
    }
}

constexpr unsigned Bitmap(Player player, Player perspect) {
    switch ((int)player * (int)perspect) {
    case -1: return 0b10;
    case 0:  return 0b11;
    case 1:  return 0b01;
    default: return 0b00;
    }
}

template <typename View_t>
constexpr auto BoardView(const std::array<View_t, BOARD_SIZE>& src, Position move, Direction dir) {
    std::vector<View_t*> view_ptrs(SAMPLE_LEN, nullptr);
    auto [dx, dy] = DeltaXY(dir);
    for (auto i = 0, j = i - SAMPLE_LEN / 2; i < SAMPLE_LEN; ++i, ++j) {
        auto x = move.x() - dx * j, y = move.y() - dy * j;
        if ((x >= 0 && x < WIDTH) && (y >= 0 && y < HEIGHT)) {
            view_ptrs[i] = &src[Position{ x, y }];
        }
    }
    return view_ptrs;
}

constexpr size_t PatternLength(unsigned bits) {
    size_t length = 0;
    for (; bits; bits >>= 2) ++length;
    return length;
}

}



#endif // !GOMOKU_HANDICRAFTS_UTILITIES_H_