#ifndef GOMOKU_MCTS_H_
#define GOMOKU_MCTS_H_
#include "Game.h" // Gomoku::Player, Gomoku::Position, Gomoku::Board
#include <vector> // std::vector
#include <memory> // std::unique_ptr
#include <functional> // std::function
#include <any> // std::any

namespace Gomoku {

struct Node {
    /* 树结构部分 */
    Node* parent = nullptr;
    std::vector<std::unique_ptr<Node>> children = {};

    /* 棋盘状态部分 */
    Position position;
    Player player;

    /* 结点价值部分 */
    std::size_t node_visits = 0;
    double state_value = 0.0;     // 表示此结点对应的当前局面的评分
    double action_prob = 0.0;     // 表示父结点对应局面下，选择该动作的概率

    /* 辅助判断函数 */
    bool isLeaf() const { return children.empty(); }
    bool isFull(const Board& board) const { return children.size() == board.moveCounts(Player::None); }
};


class Policy {
public:
    /*
        ① simple wrapper of uct
    */
    using SelectFunc = std::function<Node*(const Node*, double)>;

    /*
        ① return self
    */
    using ExpandFunc = std::function<std::size_t(Node*, std::vector<double>)>;

    /*
        ① Board State Invariant
    */
    using EvaluateFunc = std::function<std::tuple<double, std::vector<double>>(Board&)>;

    /*
        ① Board State Reset (Symmetry with select)
    */
    using UpdateFunc = std::function<void(Node*, Board&, double)>;

    // 当其中某一项传入nullptr时，该项将使用一个默认策略初始化。
    Policy(SelectFunc = nullptr, ExpandFunc = nullptr, EvaluateFunc = nullptr, UpdateFunc = nullptr);

public:
    SelectFunc   select;
    ExpandFunc   expand;
    EvaluateFunc simulate;
    UpdateFunc   backPropogate;

    // 用以存放Actions的容器，可以在不同的Policy中表现为任何形式。
    std::any container;
};


class MCTS {
public:
    MCTS(
        Position last_move = -1,
        Player last_player = Player::White,
        Policy* policy = nullptr,
        std::size_t c_iterations = 20000,
        double c_puct = sqrt(2)
    );

    Position getNextMove(Board& board);

    // get root value
    // get root probabilities

    // 蒙特卡洛树的一轮迭代
    void playout(Board& board);

    // 选出最好手，使蒙特卡洛树往下走一层
    Node* stepForward(); 

    // 根据提供的动作往下走
    Node* stepForward(Position); 

public:
    std::unique_ptr<Node> m_root;
    std::unique_ptr<Policy> m_policy;
    std::size_t m_size;
    std::size_t c_iterations;
    double      c_puct;
};

}

#endif // !GOMOKU_MCTS_H_