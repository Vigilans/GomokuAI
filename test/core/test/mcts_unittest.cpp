#include "pch.h"
#include "core/include/MCTS.h"

using namespace Gomoku;

class MCTSTest : public ::testing::Test {
protected:
    Board board;
    MCTS b_mcts = MCTS(Player::Black);
    MCTS w_mcts = MCTS(Player::White);
};

TEST_F(MCTSTest, OnePlayOut) {
    board.applyMove(b_mcts.getNextMove(board));
    w_mcts.playout(board);
}