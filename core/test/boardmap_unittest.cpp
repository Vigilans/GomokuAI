#include "pch.h"
#include "lib/include/Pattern.h"
#include <string>

using namespace Gomoku;
using namespace std;

class BoardMapTest : public ::testing::Test {
protected:
    BoardMap boardMap;
};

// 利用边界检测初始条件的LineView是否正确
TEST_F(BoardMapTest, InitialLineView) {
    for (auto direction : Directions) {
        EXPECT_EQ(boardMap.lineView({ 7,7 }, direction), string(TARGET_LEN, '-'));
    }
}

TEST_F(BoardMapTest, UpdateMove) {
    Position kifu[] = { { 7,7 },{ 7,8 },{ 6,7 },{ 8,7 }, { 9,6 } };
    boardMap.applyMove(kifu[0]);
    for (auto direction : Directions) {
        EXPECT_EQ(boardMap.lineView({ 7,7 }, direction), "------x------");
    }
    boardMap.applyMove(kifu[1]);
    EXPECT_EQ(boardMap.lineView({ 7,7 }, Direction::Horizontal), "------xo-----");
    for (auto direction : { Direction::Vertical, Direction::LeftDiag, Direction::RightDiag }) {
        EXPECT_EQ(boardMap.lineView({ 7,7 }, direction), "------x------");
    }
    boardMap.applyMove(kifu[2]);
    EXPECT_EQ(boardMap.lineView({ 7,7 }, Direction::Horizontal), "------xo-----");
    EXPECT_EQ(boardMap.lineView({ 7,7 }, Direction::Vertical), "-----xx------");
    EXPECT_EQ(boardMap.lineView({ 7,8 }, Direction::LeftDiag), "-----xo------");
    EXPECT_EQ(boardMap.lineView({ 6,7 }, Direction::LeftDiag), "------xo-----");
    for (auto direction : { Direction::LeftDiag, Direction::RightDiag }) {
        EXPECT_EQ(boardMap.lineView({ 7,7 }, direction), "------x------");
    }
    boardMap.applyMove(kifu[3]);
    EXPECT_EQ(boardMap.lineView({ 7,7 }, Direction::Vertical), "-----xxo-----");
    EXPECT_EQ(boardMap.lineView({ 8,7 }, Direction::RightDiag), "------oo-----");
    for (auto direction : { Direction::LeftDiag, Direction::RightDiag }) {
        EXPECT_EQ(boardMap.lineView({ 7,7 }, direction), "------x------");
    }
    boardMap.applyMove(kifu[4]);
    EXPECT_EQ(boardMap.lineView({ 8,7 }, Direction::RightDiag), "-----xoo-----");
    boardMap.revertMove();
    EXPECT_EQ(boardMap.lineView({ 7,7 }, Direction::Vertical), "-----xxo-----");
    EXPECT_EQ(boardMap.lineView({ 8,7 }, Direction::RightDiag), "------oo-----");
    for (auto direction : { Direction::LeftDiag, Direction::RightDiag }) {
        EXPECT_EQ(boardMap.lineView({ 7,7 }, direction), "------x------");
    }
    boardMap.revertMove(3);
    for (auto direction : Directions) {
        EXPECT_EQ(boardMap.lineView({ 7,7 }, direction), "------x------");
    }
}
