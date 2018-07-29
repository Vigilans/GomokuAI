#include "Mapping.h"
#include "Pattern.h"
#include "utils/Persistence.h"
#include <random>

using namespace std;
using namespace Gomoku;

/* ------------------- BoardMap类实现 ------------------- */

tuple<int, int> BoardMap::ParseIndex(Position pose, Direction direction) {
    // 由于前与后MAX_PATTERN_LEN - 1位均为'?'（越界位），故需设初始offset
    switch (int offset = MAX_PATTERN_LEN - 1; direction) {
        case Direction::Horizontal: // 0 + y∈[0, HEIGHT) | x: 0 -> WIDTH
            return { pose.y(), offset + pose.x() };
        case Direction::Vertical:   // HEIGHT + x∈[0, WIDTH) | y: 0 -> HEIGHT
            return { HEIGHT + pose.x(), offset + pose.y() };
        case Direction::LeftDiag:   // (WIDTH + HEIGHT) + (HEIGHT - 1) + x-y∈[-(HEIGHT - 1), WIDTH) | min(x, y): (x, y) -> (x+1, y+1)
            return { WIDTH + 2 * HEIGHT - 1 + pose.x() - pose.y(), offset + std::min(pose.x(), pose.y()) };
        case Direction::RightDiag:  // 2*(WIDTH + HEIGHT) - 1 + x+y∈[0, WIDTH + HEIGHT - 1) | min(WIDTH - 1 - x, y): (x, y) -> (x-1, y+1)
            return { 2 * (WIDTH + HEIGHT) - 1 + pose.x() + pose.y(), offset + std::min(WIDTH - 1 - pose.x(), pose.y()) };
        default:
            throw direction;
    }
}

BoardMap::BoardMap(Board* board) : m_board(board ? board : new Board) {
    this->reset();
}

string_view BoardMap::lineView(Position pose, Direction direction) {
    const auto [index, offset] = ParseIndex(pose, direction);
    return string_view(&m_lineMap[index][offset - TARGET_LEN / 2],  TARGET_LEN);
}

// WARNING: applyMove与revertMove未进行有效性检查，需在boardMap被调用之前自行检查
Player BoardMap::applyMove(Position move) {
    for (auto direction : Directions) {
        const auto [index, offset] = ParseIndex(move, direction);
        m_lineMap[index][offset] = EncodeCharset(m_board->m_curPlayer == Player::Black ? 'x' : 'o');
    }
	m_hash ^= BoardHash::HashPose(move, Player::None);
	m_hash ^= BoardHash::HashPose(move, m_board->m_curPlayer);
    return m_board->applyMove(move, false);
}

Player BoardMap::revertMove(size_t count) {
    for (int i = 0; i < count; ++i) {
		auto move = m_board->m_moveRecord.back();
        for (auto direction : Directions) {
            const auto [index, offset] = ParseIndex(move, direction);
            m_lineMap[index][offset] = EncodeCharset('-');
        }
        m_board->revertMove();
		m_hash ^= BoardHash::HashPose(move, m_board->m_curPlayer);
		m_hash ^= BoardHash::HashPose(move, Player::None);
    }
    return m_board->m_curPlayer;
}

void BoardMap::reset() {
    m_hash = 0ul;
    m_board->reset();
    for (auto& line : m_lineMap) {
        line.resize(MAX_PATTERN_LEN - 1, EncodeCharset('?')); // 线前填充越界位('?')
    }
	for (auto i = 0; i < BOARD_SIZE; ++i) {
		for (auto direction : Directions) {
			auto[index, _] = ParseIndex(i, direction);
			m_lineMap[index].push_back(EncodeCharset('-')); // 按每个位置填充空位('-')
		}
		m_hash ^= BoardHash::HashPose(i, Player::None);
	}
    for (auto& line : m_lineMap) {
        line.append(MAX_PATTERN_LEN - 1, EncodeCharset('?')); // 线后填充越界位('?')
    }
}

/* ------------------- BoardHash类实现 ------------------- */

const array<std::uint64_t[3], BOARD_SIZE> BoardHash::Zorbrist = []() {
	array<std::uint64_t[3], BOARD_SIZE> keys;
	auto data = Persistence::Load("zobrist");
	if (data.is_null()) {
		uniform_int_distribution<std::uint64_t> rnd;
		mt19937_64 eng((random_device())()); // 64位引擎
		for (int i = 0; i < BOARD_SIZE; ++i) {
			data.push_back({ rnd(eng), rnd(eng), rnd(eng) });
		}
		Persistence::Save("zobrist", data); // data不是很大，且下面还要用，故不需move
	}
	for (int i = 0; i < BOARD_SIZE; ++i) {
		std::copy(begin(data[i]), end(data[i]), begin(keys[i]));
	}
	return keys;
}();