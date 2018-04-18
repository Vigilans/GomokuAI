#ifndef GOMOKU_MCTS_H_
#define GOMOKU_MCTS_H_
#include "Game.h" // Gomoku::Player, Gomoku::Position, Gomoku::Board
#include <vector> // std::vector
#include <memory> // std::unique_ptr
#include <functional> // std::function
#include <any> // std::any

#ifdef _DEBUG
#define C_ITERATIONS 2000
#else
#define C_ITERATIONS 50000
#endif

namespace Gomoku {

// 蒙特卡洛树结点。
// 由于整个树的结点数量十分庞大，因此其内存布局务必谨慎设计。
// 32位下，sizeof(Node) == 32；64位下为56。
struct Node {
    /* 
        树结构部分 - 父结点。
        结点为观察指针，不对其拥有所有权。
    */
    Node* parent = nullptr;

    /* 
        棋盘状态部分：
          * position: 当前局面下最后一手下的位置。
          * player:   当前局面下最后一手下的玩家。
    */
    Position position = Position(-1);
    Player player = Player::None;

    /* 
        结点价值部分：
          * state_value: 结点对应局面对于结点对应玩家的价值。一般为胜率。
          * action_prob: 在父结点对应的局面下，选择该动作的概率。
    */
    float state_value = 0.0;
    float action_prob = 0.0;
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

    /* 辅助函数 */
    bool isLeaf() const { return children.empty(); }
    bool isFull(const Board& board) const { return children.size() == board.moveCounts(Player::None); }
};


class Policy {
public:
    /*
        Tree-Policy中的选择阶段策略函数。
    */
    using SelectFunc = std::function<Node*(const Node*, double expl_param)>;

    /*
        Tree-Policy中的扩展策略函数：
        ① 直接扩展一整层结点，并根据EvalResult中的概率向量为每个结点赋初值。
        ② 已经有子的位置概率应为0，防止被添加进树中。可以额外加一层在正常情况下会被短路的检查。
        ③ 返回值为新增的结点数。
    */
    using ExpandFunc = std::function<size_t(Node*, Board&, std::vector<double> action_probs)>;

    /*
        当Tree-Policy抵达中止点时，用于将棋下完（可选）并评估场面价值的Default-Policy：
        ① 在函数调用前后，一般应保证棋盘的状态不变。
        ② 返回值为 <场面价值, 各处落子的概率> 。
    */
    using EvalResult = std::tuple<double, std::vector<double>>;
    using EvalFunc = std::function<EvalResult(Board&)>;

    /*
        Tree-Policy中的反向传播策略函数：
        ① 此阶段棋盘将被重置回初始状态（与Select过程的路线对称）
        ② 结点的各种属性均在此函数中被更新。
    */
    using UpdateFunc = std::function<void(Node*, Board&, double state_value)>;

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
        size_t   c_iterations = C_ITERATIONS,
        double   c_puct       = 8//sqrt(2)
    );

    Position getNextMove(Board& board);

    // Tree-policy的评估函数
    Policy::EvalResult evalState(Board& board);
    
    // 将蒙特卡洛树往深推进一层
    Node* stepForward();                      // 选出子结点中的最好手
    Node* stepForward(Position next_move);    // 根据提供的动作往下走

    // 蒙特卡洛树的一轮迭代
    size_t playout(Board& board);

    // 重置蒙特卡洛树
    void reset();

public:
    std::unique_ptr<Node> m_root;
    std::unique_ptr<Policy> m_policy;
    size_t m_size; // FIXME: 由于unique_ptr的自动销毁，目前暂不能正确计数。
    size_t c_iterations;
    double c_puct;
};

}

#endif // !GOMOKU_MCTS_H_