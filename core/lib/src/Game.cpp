#include "Game.h"

using namespace std;
using namespace Gomoku;

// 由于是内联使用，不暴露成外部接口，因此无需进行额外参数检查，下同
inline void setState(Board* board, Player player, Position position) {
    board->moveStates(player)[position] = true;
    board->moveCounts(player) += 1;
}

inline void unsetState(Board* board, Player player, Position position) {
    board->moveStates(player)[position] = false;
    board->moveCounts(player) -= 1;
}

Board::Board() {
    moveStates(Player::None).fill(true);
    moveCounts(Player::None) = moveStates(Player::None).size();
}

Player Board::applyMove(Position move, bool checkVictory) {
    // 检查游戏是否未结束且为有效的一步
    if (m_curPlayer != Player::None && checkMove(move)) {
        setState(this, m_curPlayer, move);
        unsetState(this,Player::None, move);
        // 若checkVictory为false,则checkGameEnd()被短路。
        m_curPlayer = checkVictory && checkGameEnd(move) ? Player::None : -m_curPlayer;
    }
    return m_curPlayer;
}

Player Board::revertMove(Position move) {
    // 需要保证游戏结束后(m_curPlayer == Player::None)，也能调用成功
    if (!moveStates(Player::None)[move]) {
        m_winner = Player::None; // 由于悔了一步，游戏回到没有赢家的状态
        m_curPlayer = moveStates(Player::Black)[move] ? Player::Black : Player::White;
        setState(this, Player::None, move);
        unsetState(this, m_curPlayer, move);
    }
    return m_curPlayer;
}

Position Board::getRandomMove() const {
    if (moveCounts(Player::None) == 0) {
        throw overflow_error("board is already full");
    }
    int divisor = (RAND_MAX + 1) / BOARD_SIZE;
    int rnd = rand() % divisor;
    while (!moveStates(Player::None)[rnd]) {
        rnd = (rnd + 1) % moveStates(Player::None).size();
    }
    return Position(rnd);
}

bool Board::checkMove(Position move) {
    return move.id >= 0 && move.id < BOARD_SIZE && moveStates(Player::None)[move];
}

bool Board::checkGameEnd(Position move) {
    const int curX = move.x(), curY = move.y();
        
    // 沿不同方向的搜索方法复用
    const auto search = [&](int dx, int dy) {
        int renju = 1; // 当前落子构成的最大连珠数

        // 正向与反向搜索
        for (int sgn = 1; ; sgn = -1) {
            int x = curX, y = curY;
            for (int i = 0; i < 4; ++i) {
                x += sgn*dx, y += sgn*dy;
                // 判断坐标的格子是否未越界且归属为当前棋子
                if ((x >= 0 && x < WIDTH) && (y >= 0 && y < HEIGHT)
                    && moveStates(m_curPlayer)[y*WIDTH + x]) ++renju;
                else break;
            }
            if (sgn == -1) break;
        }

        return renju >= MAX_RENJU;
    };
        
    // 从 左上->右下 || 左下->右上 || 左->右 || 下->上  进行搜索
    if (search(1, -1) || search(1, 1) || search(1, 0) || search(0, 1)) {
        m_winner = m_curPlayer; // 赢家为当前玩家
        return true;
    } else if (moveCounts(Player::None) == 0) {
        m_winner = Player::None; // 若未赢，之后也没有可下之地，则为和局
        return true;
    } else {
        return false;
    }
}