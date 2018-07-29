#ifndef GOMOKU_MCTS_H_
#define GOMOKU_MCTS_H_
#include "Game.h" // Gomoku::Player, Gomoku::Position, Gomoku::Board
#include <vector>      // std::vector
#include <memory>      // std::unique_ptr
#include <chrono>      // std::milliseconds
#include <functional>  // std::function
#include <Eigen/Dense> // Eigen::VectorXf

namespace Gomoku {

using std::chrono::milliseconds;
using std::chrono_literals::operator""ms;

inline namespace Config {
    // 蒙特卡洛树相关的默认配置
    constexpr double C_PUCT = 5.0;
    constexpr size_t C_ITERATIONS = 10000;
    constexpr milliseconds C_DURATION = 1000ms;
}

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
    Position position = Position::npos;
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
    Node() = default;
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
    using SelectFunc = std::function<Node*(const Node*)>;
    SelectFunc select;

    /*
        Tree-Policy中的扩展策略函数：
        ① 直接扩展一整层结点，并根据EvalResult中的概率向量为每个结点赋初值。
        ② 已经有子的位置概率应为0，防止被添加进树中。可以额外加一层在正常情况下会被短路的检查。
        ③ 返回值为新增的结点数。
    */
    using ExpandFunc = std::function<size_t(Node*, Board&, const Eigen::VectorXf)>;
    ExpandFunc expand;

    /*
        当Tree-Policy抵达中止点时，用于将棋下完（可选）并评估场面价值的Default-Policy：
        ① 在函数调用前后，一般应保证棋盘的状态不变。
        ② 返回值为 <场面价值, 各处落子的概率/得分> 。
    */
    using EvalResult = std::tuple<float, const Eigen::VectorXf>;
    using EvalFunc = std::function<EvalResult(Board&)>;
    EvalFunc simulate;

    /*
        Tree-Policy中的反向传播策略函数：
        ① 此阶段棋盘将被重置回初始状态（与Select过程的路线对称）
        ② 结点的各种属性均在此函数中被更新。
    */
    using UpdateFunc = std::function<void(Node*, Board&, double)>;
    UpdateFunc backPropogate;

public:
    // 当其中某一项传入nullptr时，该项将使用一个默认策略初始化。
    Policy(SelectFunc = nullptr, ExpandFunc = nullptr, EvalFunc = nullptr, UpdateFunc = nullptr, double = C_PUCT);

    // 用于多态生成树节点的Factory函数。
    virtual std::unique_ptr<Node> createNode(Node* parent, Position pose, Player player, float value, float prob);

    // 在执行所有Playout前的准备操作。
    virtual void prepare(Board& board); 

    // 在执行所有Playout后的清理工作。
    virtual void cleanup(Board& board); 

    // 下棋策略。若策略类采用缓存机制，可在该处同时完成下棋悔棋工作。
    virtual Player applyMove(Board& board, Position move);
    
    // 悔棋策略。若策略类采用缓存机制，可以不用在此悔棋。
    virtual Player revertMove(Board& board, size_t count = 1);
    
    // 判断游戏结束的策略。若策略类有更好的方案，可在此替代。
    virtual bool checkGameEnd(Board& board);

    // 重置策略类。一般在不应通过prepare(board)恢复状态时复写。将被MCTS::reset调用。
    // prepare在每回合都会调用，而reset在一整局游戏至多调用一次。
    virtual void reset() { }

public: // 共通属性
    double c_puct; // PUCT公式的Exploit-Explore平衡因子
    size_t m_initActs = 0; // MCTS的一轮Playout开始时，Board已下的棋子数。
};


class MCTS {
public:
    // 通过时间控制模拟迭代。为默认构造方法。
    MCTS(
        milliseconds c_duration  = C_DURATION,
        Position     last_move   = -1,
        Player       last_player = Player::White,
        std::shared_ptr<Policy> policy = nullptr
    );

    // 通过次数控制模拟迭代。
    MCTS(
        size_t   c_iterations,
        Position last_move   = -1,
        Player   last_player = Player::White,
        std::shared_ptr<Policy> policy = nullptr
    );

    Position getAction(Board& board);
    Policy::EvalResult evalState(Board& board); // Tree-policy的评估函数
    
    // 将蒙特卡洛树往深推进一层
    Node* stepForward();                      // 选出子结点中的最好手
    Node* stepForward(Position next_move);    // 根据提供的动作往下走

    void syncWithBoard(Board& board); // 同步MCTS与棋盘，使得树的根节点为棋盘的最后一手
    void reset(); // 重置蒙特卡洛树与其所用的策略

private:
    // 蒙特卡洛树的一轮迭代
    size_t playout(Board& board);

    void runPlayouts(Board& board);

public:
    std::shared_ptr<Policy> m_policy;
    std::unique_ptr<Node> m_root;
    size_t m_size; // FIXME: 由于unique_ptr的自动销毁，目前暂不能正确计数。
    size_t m_iterations;
    milliseconds m_duration;

private:
    enum class Constraint {
        Iterations, Duration
    } c_constraint;
};

}

#endif // !GOMOKU_MCTS_H_