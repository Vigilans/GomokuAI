#include "pch.h"
#include "lib/include/MCTS.h"

using namespace Gomoku;

class MCTSTest : public ::testing::Test {
protected:
    Board board;
    MCTS b_mcts = MCTS();
    //MCTS w_mcts = MCTS();
};

TEST_F(MCTSTest, OnePlayOut) {
    board.applyMove(b_mcts.getNextMove(board));
    //w_mcts.playout(board);
}

TEST_F(MCTSTest, SimulateSymmetry) {
    Board board_cpy(board);
    b_mcts.m_policy->simulate(board);
    ASSERT_EQ(board, board_cpy);
}