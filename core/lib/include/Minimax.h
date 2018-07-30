#ifndef GOMOKU_MINIMAX_H_
#define GOMOKU_MINIMAX_H_
#include "Game.h" // Gomoku::Player, Gomoku::Position, Gomoku::Board
#include "pattern.h"
#include "algorithms/Heuristic.hpp"
#include <vector>      // std::vector
#include <memory>      // std::unique_ptr
#include <chrono>      // std::milliseconds
#include <functional>  // std::function
#include <Eigen/Dense> // Eigen::VectorXf

namespace Gomoku {

// using std::chrono::milliseconds;
// using std::chrono_literals::operator""ms;
	using Heuristic = Algorithms::Heuristic;

inline namespace Config {
    // Minimax系相关的默认配置
    constexpr int DEPTH = 5; //Minimax树搜索深度
}

// Minimax树结点。 [VALID]
struct MinimaxNode {
    /* 
        树结构部分 - 父结点。
        结点为观察指针，不对其拥有所有权。
    */
    MinimaxNode* parent = nullptr;

    /* 
        棋盘状态部分：
          * position: 当前局面下最后一手下的位置。
          * player:   当前局面下最后一手下的玩家。
    */
    Position position = Position(-1);
    Player player = Player::None;

    /* 
        结点价值部分：
          * current_state_value: 结点对应局面对于结点对应玩家的价值, 用于启发式排序
          * final_state_value: 对应最小/最大子节点的值
    */
    float current_state_value = 0.0;
    float final_state_value = 0.0;
    Eigen::VectorXf action_probs;
    /*
        树结构部分 - 子结点。
        结点为独有指针集合，对每个子结点拥有所有权。
    */
    std::vector<std::unique_ptr<MinimaxNode>> children = {};

    /* 
        构造与赋值函数。
        显式声明两个函数的移动版本，以阻止复制版本的自动生成。 
    */
    MinimaxNode() = default;
	MinimaxNode(MinimaxNode&&) = default;
	MinimaxNode& operator=(MinimaxNode&&) = default;

    /* 辅助函数 */
    bool isLeaf() const { return children.empty(); }
    bool isFull(const Board& board) const { return children.size() == board.moveCounts(Player::None); }
};


class MinimaxPolicy {
public:
    /*
        Tree-Policy中的扩展策略函数：
        ① 直接扩展一整层结点，并根据EvalResult中的场面价值向量为每个结点赋初值。最先检查当前判分高的节点并进行depth-first-search
        ② 返回值为新增的结点数。
    */
   
    using ExpandFunc = std::function<size_t(MinimaxNode*, Board&, const Eigen::VectorXf)>;
    ExpandFunc expand;

    /*
        当Tree-Policy抵达中止点时，用于将棋下完（可选）并评估场面价值的Default-Policy：
        ① 在函数调用前后，一般应保证棋盘的状态不变。
        ② 返回值为 <场面价值, 各处落子的概率/得分> 。
    */
    using EvalResult = std::tuple<float, const Eigen::VectorXf>;
    using EvalFunc = std::function<EvalResult(Board&)>;
    EvalFunc evaluate;

    /*
        Tree-Policy中的反向传播策略函数：
        ① 此阶段棋盘将被重置回初始状态（与Select过程的路线对称）
        ② 结点的各种属性均在此函数中被更新。
    */
    // using UpdateFunc = std::function<void(MinimaxNode*, Board&, double)>;
    // UpdateFunc backPropogate;

public:
    // 当其中某一项传入nullptr时，该项将使用一个默认策略初始化。
	MinimaxPolicy() {
		m_initActs = 0;
	};
    //Policy(ExpandFunc = nullptr, EvalFunc = nullptr, size_t = DEPTH);

    // 用于多态生成树节点的Factory	函数。
    //virtual std::unique_ptr<MinimaxNode> createNode(MinimaxNode* parent, Position pose, Player player, float value, float prob, short );
	static std::unique_ptr<MinimaxNode> createNode(MinimaxNode* parent, Position pose, Player player, float cur_value, float fin_value);
    

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
    double c_depth; //限制搜索树深度
    size_t m_initActs; // Minimax搜索树的一轮Playout开始时，Board已下的棋子数。
};


class Minimax {//Minimax树
public:
    // 通过深度控制模拟迭代。
	Minimax(
		short   c_depth,//=3,
		Position last_move,// = { Config::GameConfig::WIDTH / 2 ,Config::GameConfig::HEIGHT / 2 },
		Player   last_player// = Player::Black
	);

    Position getAction(Board& board);

	static Eigen::VectorXf evalState(std::unique_ptr<MinimaxNode>& node, Board& board) {
		Evaluator m_evaluator;
		m_evaluator.syncWithBoard(board);
		auto action_probs = Heuristic::EvaluationProbs(m_evaluator, board.m_curPlayer);
		Heuristic::DecisiveFilter(m_evaluator, action_probs);
		node->current_state_value = Heuristic::EvaluationValue(m_evaluator, board.m_curPlayer, 1, true);
		return action_probs;
	}    
    
	// 将Minimax树往深推进一层
    MinimaxNode* stepForward();                      // 选出子结点中的最好手
    MinimaxNode* stepForward(Position next_move);    // 根据提供的动作往下走

    void syncWithBoard(Board& board); // 同步Minimax搜索树与棋盘，使得树的根节点为棋盘的最后一手
    void reset(); // 重置minimax树与其所用的策略

private:
    // Minimax树的一轮迭代 

    void runPlayouts(Board& board);


public:
    std::shared_ptr<MinimaxPolicy> m_policy;
    std::unique_ptr<MinimaxNode> m_root; //根节点
    size_t m_depth; // minimax树搜索深度，默认为DEPTH
    Evaluator m_evaluator;
    double m_alpha;
    double m_beta;

private:
    enum class Constraint {
        Depth,Duration
    } c_constraint;
};

}

#endif // !GOMOKU_MCTS_H_