#ifndef GOMOKU_MCTS_H_
#define GOMOKU_MCTS_H_
#include "Game.h" // Gomoku::Player, Gomoku::Position, Gomoku::Board
#include <vector> // std::vector
#include <memory> // std::unique_ptr

namespace Gomoku {

// 前置声明
struct Policy;

struct Node {
    Node* parent = nullptr;
    std::vector<std::unique_ptr<Node>> children = {};
    Position position = -1;
    std::size_t visits = 1; // 创建时即被认为已访问过一次
    float victory_value = 0.0;
    float ucb1 = 0.0;

    bool isLeaf() const {
        return children.empty();
    }

    double UCB1(double expl_param) {
        return victory_value / visits + expl_param * sqrt(log(parent->visits) / visits);
    }
};


class MCTS {
public:
    MCTS(Player player, Position lastMove = -1);

    Position getNextMove(Board& board);

public:
    // 蒙特卡洛树的一轮迭代
    void playout(Board& board);

    // 选出最好手，使蒙特卡洛树往下走一层
    void stepForward();

private:
    Node* select(const Node* node) const;

    void expand(Node* node, const Board& board);

    double simulate(Node* node, Board& board);

    void backPropogate(Node* node, Board& board, double value);

private:
    Player m_player;
    std::size_t m_iterations;
    std::unique_ptr<Node> m_root;  
    std::vector<Position> m_moveStack;
};

}

#endif // !GOMOKU_MCTS_H_

