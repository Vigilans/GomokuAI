#ifndef GOMOKU_HANDICRAFTS_EVALUATOR_H_
#define GOMOKU_HANDICRAFTS_EVALUATOR_H_
#include "Pattern.h"
#include "MSTree.h"
#include <unordered_map>

namespace Gomoku::Handicrafts {

/*
    TODO:
      1. Pattern的空位变成三种('_', '^'及'-')
      2. 直接在Pattern中指明分数
      3. 添加Bitmap类，每行/每列/每斜边维护一个Bitset。
*/
class Evaluator {
public:
    // 在不同的Evaluator间共享的变量
    static std::unordered_map<unsigned, Pattern, BitmapHasher> Patterns;
    static MSTree<unsigned, Pattern> patterns;
    static std::vector<unsigned> Masks;

    std::array<int, BOARD_SIZE> m_distributions[2][(int)PatternType::Size - 1]; // 不统计五子连珠分布
    Board m_board; // 需要内部维护一个Board,以免受到外部变动的干扰。
    
public:
    Evaluator();

    auto& distribution(Player player, PatternType type) { return m_distributions[player == Player::Black][(int)type]; }

    int patternCount(Position move, PatternType type, Player perspect, Player curPlayer);

    // 同步Evaluator至传入的Board状态。
    void syncWithBoard(const Board& board);

    unsigned getBitmap(Position move, Direction dir, Player perspect);

    void reset();

    Player applyMove(Position move);

    Player revertMove(size_t count = 1);

    // 利用Evaluator，我们可以做到快速检查游戏是否结束。
    bool checkGameEnd();

    void updateOnePiece(int delta, Position move, Player src_player);

private:
    void initDistributions();

    void updateMove(Position move, Player src_player);

    void updatePattern(unsigned bitmap, int delta, Position move, Direction dir, Player perspect);
};

}

#endif // !GOMOKU_HANDICRAFTS_EVALUATOR_H_