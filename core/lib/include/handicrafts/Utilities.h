#ifndef GOMOKU_HANDICRAFTS_UTILITIES_H_
#define GOMOKU_HANDICRAFTS_UTILITIES_H_
#include "../Game.h"
#include <Eigen/Dense>

namespace Gomoku::Handicrafts {

enum Config {
    MAX_PATTERN_LEN = 6,
    MAX_SURROUNDING_SIZE = 5,
    SAMPLE_LEN = 2 * MAX_PATTERN_LEN + 1,
    HASH_LEN = 2 * (SAMPLE_LEN + 1) / 3
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

/*
    * Encoding a pattern to a bitmap, making it suitable for computing
      and serving as hash index.
    * Pattern format : "{$perspect}{$pattern}"
        perspect:
          binary state indicating the perspect of view.
          '+' for black, '-' for white.
        pattern: 
          list of chars representing player states.
          'x' for black, 'o' for white.
          '_' for valuable blank,
          '^' for invaluable blank.
*/
constexpr unsigned Encode(std::string_view str) {
    Player perspect = Player::None;
    switch (str[0]) {
    case '+': perspect = Player::Black; break;
    case '-': perspect = Player::White; break;
    }
    unsigned bitmap = 0u;
    for (auto i = 1; i < str.length(); ++i) {
        auto piece = 0b00u;
        switch (str[i]) {
        case 'x': 
            piece = Bitmap(Player::Black, perspect); break;
        case 'o': 
            piece = Bitmap(Player::White, perspect); break;
        case '_':
        case '^':
            piece = Bitmap(Player::None, perspect); break;
        }
        bitmap <<= 2;
        bitmap |= piece;
    }
    return bitmap;
}

/*
    * Convert a bitmap to a human-readable string.
      'x' for black, 'o' for white, '-' for blank, '?' for invalid states.
    * NOTE: encode & decode is not symmetric! 
            information will be lost during encoding. 
*/
inline std::string Decode(unsigned bits, Player perspect = Player::Black) {
    std::string result;
    for (auto i = 0; i < SAMPLE_LEN; ++i) {
        switch (bits & 0b11u) {
        case 0b00: result.push_back('?'); break;
        case 0b01: result.push_back(perspect == Player::Black ? 'x' : 'o'); break;
        case 0b10: result.push_back(perspect == Player::Black ? 'o' : 'x'); break;
        case 0b11: result.push_back('-'); break;
        }
        bits >>= 2;
    }
    return result;
}

struct BitmapHasher {
    std::size_t operator()(unsigned bits) const {
        std::size_t hash = 0;
        hash ^= bits;
        hash ^= (bits >>= HASH_LEN );
        hash ^= (bits >>= HASH_LEN - 1) + 1;
        hash &= (1 << HASH_LEN) - 1;
        return hash;
    }
};

template <size_t Length = SAMPLE_LEN, typename View_t>
inline auto& LineView(std::array<View_t, BOARD_SIZE>& src, Position move, Direction dir) {
    static std::vector<View_t*> view_ptrs(Length);
    auto [dx, dy] = DeltaXY(dir);
    for (int i = 0, j = i - Length / 2; i < Length; ++i, ++j) {
        auto x = move.x() + dx * j, y = move.y() + dy * j;
        if ((x >= 0 && x < WIDTH) && (y >= 0 && y < HEIGHT)) {
            view_ptrs[i] = &src[Position{ x, y }];
        } else {
            view_ptrs[i] = nullptr;
        }
    }
    return view_ptrs;
}

template <int Size = MAX_SURROUNDING_SIZE, typename View_t>
inline auto BlockView(std::array<View_t, BOARD_SIZE>& src, Position move) {
    static Eigen::Map<Eigen::Matrix<View_t, HEIGHT, WIDTH>> view(nullptr);
    new (&view) decltype(view)(src.data()); // placement new
    auto left_bound  = std::max(move.x() - Size / 2, 0);
    auto right_bound = std::min(move.x() + Size / 2, WIDTH - 1);
    auto up_bound    = std::max(move.y() - Size / 2, 0);
    auto down_bound  = std::min(move.y() + Size / 2, HEIGHT - 1);
    Position lr{ left_bound, up_bound }, rd{ right_bound, down_bound };
    Position origin = move - Position{ Size / 2, Size / 2 };
    return std::make_tuple(
        view.block(lr.x(), lr.y(), rd.x() - lr.x() + 1, rd.y() - lr.y() + 1),
        Position(lr - origin), Position(rd - origin)
    );
}

}



#endif // !GOMOKU_HANDICRAFTS_UTILITIES_H_
