#include "pch.h"
#include "lib/include/Game.h"

using namespace Gomoku;

// Problem 1 found: x坐标一定不能是负数
// Problem 2 found: 公式只有在同正同负下才能起作用
class PositionTest : public ::testing::Test {
protected:
    static const int caseSize = 10;

    virtual void SetUp() override {
        for (int i = 0; i < caseSize; ++i) {
            int sgn = randSgn(); // 每轮循环保证符号一致
            xCoords[i] = sgn * (rand() % WIDTH);
            yCoords[i] = sgn * (rand() % HEIGHT);
            ids[i] = sgn * (rand() % GameConfig::BOARD_SIZE);
        }
    }

    int randSgn() {
        return 2 * (rand() % 2) - 1;
    }
    
    int xCoords[caseSize];
    int yCoords[caseSize];
    int ids[caseSize];
};

TEST_F(PositionTest, CtorSymmetry) {
    for (auto id : ids) {
        Position pos(id);
        ASSERT_EQ(Position(pos.x(), pos.y()), pos);
    }
}

TEST_F(PositionTest, CheckFormula) {
    // 包含了负数情况。根据(a/b)*b + a%b = a的规则，
    // 就算x或y是负数，公式也依然应当成立。
    for (auto id : ids) {
        Position pos(id);
        int offset = rand() % 500;
        ASSERT_EQ(Position(pos.x(), pos.y() + offset).id, pos.id + WIDTH * offset);
        ASSERT_EQ(Position(pos.x() + offset, pos.y()).id, pos.id + offset);
    }
}

TEST_F(PositionTest, CheckXY) {
    for (int i = 0; i < caseSize; ++i) {
        Position pos(xCoords[i], yCoords[i]);
        ASSERT_EQ(pos.x(), xCoords[i]);
        ASSERT_EQ(pos.y(), yCoords[i]);
    }
}