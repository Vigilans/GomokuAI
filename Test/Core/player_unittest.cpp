#include "pch.h"
#include "../../Core/src/Game.hpp"

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
                continue; // ATTENTION
            }
            if (player == winner) {
                ASSERT_GT(getFinalScore(player, winner), 0);
            } else {
                ASSERT_LE(getFinalScore(player, winner), 0);
            }
        }
    }
}