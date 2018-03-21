#ifndef GOMOKU_MCTS_H_
#define GOMOKU_MCTS_H_
#include "Game.h" // Gomoku::Player, Gomoku::Position, Gomoku::Board
#include <vector> // std::vector
#include <memory> // std::unique_ptr
#include <functional> // std::function

namespace Gomoku {

struct Node {
    /* 树结构部分 */
    Node* parent = nullptr;
    std::vector<std::unique_ptr<Node>> children = {};

    /* 棋盘状态部分 */
    Position position;
    Player player;

    /* 结点价值部分 */
    std::size_t visits = 1; // 创建时即被认为已访问过一次
    float victory_value = 0.0;
    float probability = 0.0;

    /* 辅助判断函数 */
    bool isLeaf() const { return children.empty(); }
    bool isFull(const Board& board) const { return children.size() == board.moveCounts(Player::None); }
};


struct Policy {
    // 提供默认的Policy函数，函数签名不一定与最终函数一致。
    struct Default {
        static double ucb1(const Node* node, double expl_param);
        static Node*  select(const Node* node, std::function<double(const Node*, double)> ucb1);
        static Node*  expand(Node* node, const Board& board);
        static double simulate(Node* node, Board& board, std::vector<Position>& moveStack);
        static void   backPropogate(Node* node, Board& board, double value);
    };

    std::function<double(const Node*, double)> ucb1 = Default::ucb1;
    std::function<Node*(const Node*)>          select = [this](const Node* n) { return Default::select(n, this->ucb1); };
    std::function<Node*(Node*, const Board&)>  expand = Default::expand;
    std::function<double(Node*, Board&)>       simulate = [this](Node* n, Board& b) { return Default::simulate(n, b, this->m_stack); };
    std::function<void(Node*, Board&, double)> backPropogate = Default::backPropogate;

protected:
    std::vector<Position> m_stack;
};


class MCTS {
public:
    MCTS(Player player, Position lastMove = -1);

    Position getNextMove(Board& board);

    // 蒙特卡洛树的一轮迭代
    void playout(Board& board);

    // 选出最好手，使蒙特卡洛树往下走一层
    void stepForward();

private:
    std::unique_ptr<Node> m_root;
    std::unique_ptr<Policy> m_policy;
    std::size_t m_iterations;
    Player m_player;
};

}

#endif // !GOMOKU_MCTS_H_