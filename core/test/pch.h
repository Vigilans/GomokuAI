//
// pch.h
// Header for standard system include files.
//

#pragma once
#define GTEST_HAS_TR1_TUPLE 0 // It seems that MSVC C++17 does not provide ::std::tr1 namespace.

#include "gtest/gtest.h"
#include <array>
#include <cstdlib>
#include "lib/include/Game.h"

namespace Gomoku {

inline bool operator==(const Board& lhs, const Board& rhs) {
    // 为加快速度，只检查moveStates元素个数及棋谱记录是否相等，就不检查每个元素是否一一对应了
    // 注意m_moveStates与m_moveCounts都不是std::array，而是原生数组，因此不能直接用来比较
    auto make_tied = [](const Board& b) {
        return std::tie(b.m_curPlayer, b.m_winner, b.m_moveCounts[0], b.m_moveCounts[1], b.m_moveCounts[2], b.m_moveRecord);
    };
    return make_tied(lhs) == make_tied(rhs);
}

}