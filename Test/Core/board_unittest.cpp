#include "pch.h"
#include "../../Core/src/Game.hpp"

using namespace Gomoku;

class BoardTestF : public ::testing::Test {
protected:
    virtual void SetUp() override {

    }

};

TEST(BoardTest, MoveSymmetry) {
    Board board1, board2;
    Position pos(1, 1);
    board1.applyMove(pos);
    board1.revertMove(pos);
    EXPECT_EQ(board1.m_appliedMoves, board2.m_appliedMoves);
    EXPECT_EQ(board1.m_availableMoves, board2.m_availableMoves);
}