#include "pch.h"
#include "lib/include/MCTS.h"

using namespace Gomoku;

class MCTSTest : public ::testing::Test {
protected:
    Board board;
    MCTS mcts;
};

TEST_F(MCTSTest, OneEvaluation) {
    board.applyMove(mcts.getNextMove(board));
}

TEST_F(MCTSTest, EvaluateSymmetry) {
    Position positions[] = { { 7, 7 },{ 8, 8 },{ 9, 9 },{ 10, 10 } };
    Board board_cpy(board);
    for (auto move : positions) {
        board.applyMove(move);
        board_cpy.applyMove(move);
        mcts.stepForward(move);
        ASSERT_EQ(mcts.m_root->position, board.m_moveRecord.back()) << "Root position not equal to last move";
        mcts.m_policy->simulate(board);
        ASSERT_EQ(board, board_cpy) << "DefaultPolicy::simulate changed board state"; // board state invariant
        Position next_move = mcts.getNextMove(board);
        ASSERT_EQ(board, board_cpy) << "MCTS::getNextMove changed board state"; // board state invariant
        board.applyMove(next_move);
        board_cpy.applyMove(next_move);
    }
}