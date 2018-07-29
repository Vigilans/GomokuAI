#ifndef GOMOKU_MAPPING_H_
#define GOMOKU_MAPPING_H_
#include "Game.h"
#include <cstdint>

namespace Gomoku {

// 棋盘的四个方向的抽象封装
enum class Direction : char {
	Horizontal, Vertical, LeftDiag, RightDiag
};

// 将方向枚举解包成具体的(Δx, Δy)值
constexpr std::pair<int, int> operator*(Direction direction) {
	switch (direction) {
		case Direction::Horizontal: return { 1, 0 };
		case Direction::Vertical:   return { 0, 1 };
		case Direction::LeftDiag:   return { 1, 1 };
		case Direction::RightDiag:  return { -1, 1 };
		default: return { 0, 0 };
	}
}

// 将pose按照dir方向移动offset个单位
constexpr Position Shift(Position pose, int offset, Direction dir) {
	return { pose.id + offset * Position(*dir) };
}

constexpr Position& SelfShift(Position& pose, int offset, Direction dir) {
	return pose.id += offset * Position(*dir), pose;
}

// 数组下标与Direction枚举值一一对应
constexpr Direction Directions[] = {
	Direction::Horizontal, Direction::Vertical, Direction::LeftDiag, Direction::RightDiag
};


// 编码棋盘的棋子字符
constexpr int EncodeCharset(char ch) {
	switch (int code = 0; ch) {
		case '-': case '_': case '^': case '~': ++code; // 空位
		case '?': ++code; // 越界位
		case 'o': ++code; // 白棋
		case 'x': ++code; // 黑棋
		default: return code;
	}
}

// 棋子状态编码集
constexpr int Codeset[] = { 1, 2, 3, 4 };


class BoardMap {
public:
	static std::tuple<int, int> ParseIndex(Position pose, Direction direction);

	explicit BoardMap(Board* board = nullptr);

	std::string_view lineView(Position pose, Direction direction);

	Player applyMove(Position move);

	Player revertMove(size_t count = 1);

	void reset();

public:
	std::unique_ptr<Board> m_board;
	std::array<std::string, 3 * (WIDTH + HEIGHT) - 2> m_lineMap;
	std::uint64_t m_hash;
};


struct BoardHash {
	// Zobrist哈希，存储黑/白/无棋子三种状态
	static const std::array<std::uint64_t[3], BOARD_SIZE> Zorbrist;
	
	static std::uint64_t HashPose(Position pose, Player player) {
		return Zorbrist[pose.id][static_cast<int>(player) + 1];
	}

	// WARNING: 64位编译环境下sizeof(size_t)才能为64位
	std::size_t operator()(const BoardMap& boardMap) {
		return boardMap.m_hash;
	}
};


}

namespace std {
	template <> 
	struct hash<Gomoku::BoardMap> 
		: public Gomoku::BoardHash { };
}

#endif // !GOMOKU_MAPPING_H_