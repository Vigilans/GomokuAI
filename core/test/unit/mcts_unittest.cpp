#include "pch.h"
#include "lib/include/MCTS.h"
#include "lib/include/policies/Traditional.h"

using namespace Gomoku;
using namespace Gomoku::Policies;

#if _DEBUG
#define C_ITERATIONS 500
#else
#define C_ITERATIONS 20000
#endif

//class MCTSTest : public ::testing::Test {
//protected:
//    MCTSTest() : traditional_mcts(C_ITERATIONS, -1, Player::White, std::shared_ptr<Policy>(new TraditionalPolicy)) {}
//
//    Board board;
//    MCTS random_mcts;
//    MCTS traditional_mcts;
//};
//
//TEST_F(MCTSTest, OneEvaluation) {
//    board.applyMove(traditional_mcts.getAction(board));
//}

//TEST_F(MCTSTest, EvaluateSymmetry) {
//    Position positions[] = { { 7, 7 },{ 8, 8 },{ 9, 9 },{ 10, 10 } };
//    Board board_cpy(board);
//    for (auto move : positions) {
//        board.applyMove(move);
//        board_cpy.applyMove(move);
//        random_mcts.stepForward(move);
//        ASSERT_EQ(random_mcts.m_root->position, board.m_moveRecord.back()) << "Root position not equal to last move";
//        random_mcts.m_policy->simulate(board);
//        ASSERT_EQ(board, board_cpy) << "DefaultPolicy::simulate changed board state"; // board state invariant
//        Position next_move = random_mcts.getAction(board);
//        ASSERT_EQ(board, board_cpy) << "MCTS::getAction changed board state"; // board state invariant
//        board.applyMove(next_move);
//        board_cpy.applyMove(next_move);
//    }
//}