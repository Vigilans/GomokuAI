#include "pch.h"
#include "lib/include/Pattern.h"
#include "lib/src/ACAutomata.h"
#include <string>

using namespace Gomoku;
using namespace std;

inline string operator""_v(const char* str, size_t size) {
    string code(size, 0);
    std::transform(str, str + size, code.begin(), EncodeCharset);
    return code;
}

class BoardMapTest : public ::testing::Test {
protected:
    BoardMap boardMap;
};

// 利用边界检测初始条件的LineView是否正确
TEST_F(BoardMapTest, InitialLineView) {
    for (auto direction : Directions) {
        EXPECT_EQ(boardMap.lineView({ 7,7 }, direction), string(TARGET_LEN, EncodeCharset('-')));
    }
    EXPECT_EQ(boardMap.lineView({ 0,0 }, Direction::Horizontal), "??????-------"_v);
    EXPECT_EQ(boardMap.lineView({ 0,0 }, Direction::Vertical),   "??????-------"_v);
    EXPECT_EQ(boardMap.lineView({ 0,0 }, Direction::LeftDiag),   "??????-------"_v);
    EXPECT_EQ(boardMap.lineView({ 0,0 }, Direction::RightDiag),  "??????-??????"_v);
    EXPECT_EQ(boardMap.lineView({ 1,2 }, Direction::Horizontal), "?????--------"_v);
    EXPECT_EQ(boardMap.lineView({ 1,2 }, Direction::Vertical),   "????---------"_v);
    EXPECT_EQ(boardMap.lineView({ 1,2 }, Direction::LeftDiag),   "?????--------"_v);
    EXPECT_EQ(boardMap.lineView({ 1,2 }, Direction::RightDiag),  "????----?????"_v);
}

TEST_F(BoardMapTest, UpdateMove) {
    Position kifu[] = { { 7,7 },{ 8,7 },{ 7,6 },{ 7,8 },{ 6,9 } };
    boardMap.applyMove(kifu[0]);
    for (auto direction : Directions) {
        EXPECT_EQ(boardMap.lineView({ 7,7 }, direction), "------x------"_v);
    }
    boardMap.applyMove(kifu[1]);
    EXPECT_EQ(boardMap.lineView({ 7,7 }, Direction::Horizontal), "------xo-----"_v);
    for (auto direction : { Direction::Vertical, Direction::LeftDiag, Direction::RightDiag }) {
        EXPECT_EQ(boardMap.lineView({ 7,7 }, direction), "------x------"_v);
    }
    boardMap.applyMove(kifu[2]);
    EXPECT_EQ(boardMap.lineView({ 7,7 }, Direction::Horizontal), "------xo-----"_v);
    EXPECT_EQ(boardMap.lineView({ 7,7 }, Direction::Vertical), "-----xx------"_v);
    EXPECT_EQ(boardMap.lineView({ 8,7 }, Direction::LeftDiag), "-----xo------"_v);
    EXPECT_EQ(boardMap.lineView({ 7,6 }, Direction::LeftDiag), "------xo-----"_v);
    for (auto direction : { Direction::LeftDiag, Direction::RightDiag }) {
        EXPECT_EQ(boardMap.lineView({ 7,7 }, direction), "------x------"_v);
    }
    boardMap.applyMove(kifu[3]);
    EXPECT_EQ(boardMap.lineView({ 7,7 }, Direction::Vertical), "-----xxo-----"_v);
    EXPECT_EQ(boardMap.lineView({ 7,8 }, Direction::RightDiag), "-----oo------"_v);
    for (auto direction : { Direction::LeftDiag, Direction::RightDiag }) {
        EXPECT_EQ(boardMap.lineView({ 7,7 }, direction), "------x------"_v);
    }
    boardMap.applyMove(kifu[4]);
    EXPECT_EQ(boardMap.lineView({ 7,8 }, Direction::RightDiag), "-----oox-----"_v);
    boardMap.revertMove();
    EXPECT_EQ(boardMap.lineView({ 7,7 }, Direction::Vertical), "-----xxo-----"_v);
    EXPECT_EQ(boardMap.lineView({ 7,8 }, Direction::RightDiag), "-----oo------"_v);
    for (auto direction : { Direction::LeftDiag, Direction::RightDiag }) {
        EXPECT_EQ(boardMap.lineView({ 7,7 }, direction), "------x------"_v);
    }
    boardMap.revertMove(3);
    for (auto direction : Directions) {
        EXPECT_EQ(boardMap.lineView({ 7,7 }, direction), "------x------"_v);
    }
}
