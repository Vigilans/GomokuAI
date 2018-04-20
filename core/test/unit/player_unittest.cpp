#include "pch.h"
#include "lib/include/Game.h"

using namespace Gomoku;

TEST(PlayerTest, OperatorNegative) {
    Player players[3] = { Player::Black,  Player::White, Player::None };
    for (auto player : players) {
        ASSERT_EQ(-(-player), player);
    }
    ASSERT_EQ(-Player::Black, Player::White);
    ASSERT_EQ(-Player::White, Player::Black);
    ASSERT_EQ(-Player::None, Player::None);
}

TEST(PlayerTest, FinalScore) {
    Player players[3] = { Player::Black,  Player::White, Player::None };
    for (auto player : players) {
        for (auto winner : players) {
            if (player == Player::None) {
                continue; // ATTENTION: potential bug
            }
            if (player == winner) {
                ASSERT_GT(CalcScore(player, winner), 0);
            } else {
                ASSERT_LE(CalcScore(player, winner), 0);
            }
        }
    }
}