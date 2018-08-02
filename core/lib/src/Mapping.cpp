#include "Mapping.h"
#include "Pattern.h"
#include "utils/Persistence.h"
#include <random>

using namespace std;
using namespace Eigen;

namespace Gomoku {

/* ------------------- BoardLines类实现 ------------------- */

constexpr tuple<int, int> ParseIndex(Position pose, Direction direction) {
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

array<VectorXi, LINES_COUNT> BoardLines::PoseIDs = []() {
    array<vector<int>, LINES_COUNT> pose_lines;
    for (auto& line : pose_lines) {
        line.resize(MAX_PATTERN_LEN - 1, Position::npos); // 线前填充越界位(npos)
    }
    for (auto i = 0; i < BOARD_SIZE; ++i)
    for (auto direction : Directions) {
        auto [index, _] = ParseIndex(i, direction);
        pose_lines[index].push_back(i); // 按每个位置填充ID
    }
    for (auto& line : pose_lines) {
        line.insert(line.end(), MAX_PATTERN_LEN - 1, Position::npos); // 线后填充越界位(npos)
    }
    decltype(PoseIDs) eigen_pose_lines;
    for (int i = 0; i < LINES_COUNT; ++i) {
        eigen_pose_lines[i] = Map<VectorXi>(&pose_lines[i][0], pose_lines[i].size());
    }
    return eigen_pose_lines;
}();

VectorBlock<VectorXi> BoardLines::TargetLine(Position pose, Direction dir) {
    const auto [index, offset] = ParseIndex(pose, dir);
    return PoseIDs[index].segment(offset - TARGET_LEN / 2, TARGET_LEN);
}

BoardLines::BoardLines() {
    this->reset();
}

string_view BoardLines::operator()(Position pose, Direction direction) {
    const auto [index, offset] = ParseIndex(pose, direction);
    return string_view(&m_lines[index][offset - TARGET_LEN / 2], TARGET_LEN);
}

void BoardLines::updateMove(Position move, Player player) {
    for (auto direction : Directions) {
        const auto[index, offset] = ParseIndex(move, direction);
        m_lines[index][offset] = EncodeCharset(EncodePlayer(player));
    }
}

void BoardLines::reset() {
    for (auto& line : m_lines) {
        line.resize(MAX_PATTERN_LEN - 1, EncodeCharset('?')); // 线前填充越界位('?')
    }
    for (auto i = 0; i < BOARD_SIZE; ++i)
        for (auto direction : Directions) {
            auto[index, _] = ParseIndex(i, direction);
            m_lines[index].push_back(EncodeCharset('-')); // 按每个位置填充空位('-')
        }
    for (auto& line : m_lines) {
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

BoardHash::BoardHash() {
    this->reset();
}

void BoardHash::updateMove(Position move, Player player) {
    m_hash ^= BoardHash::HashPose(move, player);
    m_hash ^= BoardHash::HashPose(move, Player::None);
}

void BoardHash::reset() {
    m_hash = 0ul;
    for (int i = 0; i < BOARD_SIZE; ++i) {
        m_hash ^= BoardHash::HashPose(i, Player::None);
    }
}

/* ------------------- BoardMap类实现 ------------------- */

// WARNING: applyMove与revertMove未进行有效性检查
Player BoardMap::applyMove(Position move) {
    m_lineView.updateMove(move, m_board.m_curPlayer);
    m_hasher.updateMove(move, m_board.m_curPlayer);
    return m_board.applyMove(move, false);
}

Player BoardMap::revertMove(size_t count) {
    for (int i = 0; i < count; ++i) {
        auto move = m_board.m_moveRecord.back();
        m_lineView.updateMove(move, Player::None);
        m_board.revertMove();
        m_hasher.updateMove(move, m_board.m_curPlayer);
    }
    return m_board.m_curPlayer;
}

void BoardMap::reset() {
    m_board.reset();
    m_hasher.reset();
    m_lineView.reset();
}

}