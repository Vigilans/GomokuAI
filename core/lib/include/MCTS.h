#ifndef GOMOKU_MCTS_H_
#define GOMOKU_MCTS_H_
#include "Game.h" // Gomoku::Player, Gomoku::Position, Gomoku::Board
#include <vector> // std::vector
#include <memory> // std::unique_ptr
#include <functional> // std::function
#include <any> // std::any

namespace Gomoku {

struct Node {
    /* 
        树结构部分 - 父结点。
        结点为观察指针，不对其拥有所有权。
    */
    Node* parent = nullptr;

    /* 
        棋盘状态部分：
          * position: 当前局面下最近一手的位置。
          * player: 当前局面下应走一下手的玩家。
    */
    Position position = Position(-1);
    Player player = Player::None;

    /* 
        结点价值部分：
          * state_value: 此结点对应的当前局面的评分。一般为胜率。
          * action_prob: 在父结点对应的局面下，选择该动作的概率。
    */
    double state_value = 0.0;
    double action_prob = 0.0;
    size_t node_visits = 0;

    /*
        树结构部分 - 子结点。
        结点为独有指针集合，对每个子结点拥有所有权。
    */
    std::vector<std::unique_ptr<Node>> children = {};

    /* 
        构造与赋值函数。
        显式声明两个函数的移动版本，以阻止复制版本的自动生成。 
    */
    Node(Node&&) = default;
    Node& operator=(Node&&) = default;

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
    using ExpandFunc = std::function<size_t(Node*, std::vector<double>)>;

    /*
        ① Board State Invariant
    */
    using EvalResult = std::tuple<double, std::vector<double>>;
    using EvalFunc = std::function<EvalResult(Board&)>;

    /*
        ① Board State Reset (Symmetry with select)
    */
    using UpdateFunc = std::function<void(Node*, Board&, double)>;

    // 当其中某一项传入nullptr时，该项将使用一个默认策略初始化。
    Policy(SelectFunc = nullptr, ExpandFunc = nullptr, EvalFunc = nullptr, UpdateFunc = nullptr);

public:
    SelectFunc select;
    ExpandFunc expand;
    EvalFunc   simulate;
    UpdateFunc backPropogate;

public:
    std::any container; // 用以存放Actions的容器，可以在不同的Policy中表现为任何形式。
};


class MCTS {
public:
    MCTS(
        Position last_move    = -1,
        Player   last_player  = Player::White,
        Policy*  policy       = nullptr,
        size_t   c_iterations = 20000,
        double   c_puct       = sqrt(2)
    );

    Position getNextMove(Board& board);

    // Tree-policy的评估函数
    Policy::EvalResult evalState(Board& board);
    
    // 将MCTS往深推进一层
    Node* stepForward();                      // 选出子结点中的最好手
    Node* stepForward(Position next_move);    // 根据提供的动作往下走

    // 蒙特卡洛树的一轮迭代
    size_t playout(Board& board);

public:
    std::unique_ptr<Node> m_root;
    std::unique_ptr<Policy> m_policy;
    size_t m_size;
    size_t c_iterations;
    double c_puct;
};

}

#endif // !GOMOKU_MCTS_H_