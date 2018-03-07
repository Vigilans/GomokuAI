#include <ctime>
#include <cstdlib>
#include "pch.h"
#include "../../Core/src/Game.hpp"

using namespace Gomoku;

class PositionTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        srand(time(nullptr));
        for (auto& x : xCoords) {
            x = rand() % width;
        }
        for (auto& y : yCoords) {
            y = rand() % height;
        }
        for (auto& y : ids) {
            y = rand() % width * height;
        }
    }
    
    int xCoords[10];
    int yCoords[10];
    int ids[10];
};

TEST_F(PositionTest, CtorSymmetry) {
    for (auto id : ids) {
        Position pos(id);
        ASSERT_EQ(Position(pos.x(), pos.y()), pos);
    }
}

TEST_F(PositionTest, CheckFormula) {
    // WIP
}