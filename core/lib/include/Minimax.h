#ifndef GOMOKU_MINIMAX_H_
#define GOMOKU_MINIMAX_H_
#include "Mapping.h"
#include "Pattern.h"
#include <vector>      // std::vector
#include <memory>      // std::unique_ptr
#include <chrono>      // std::milliseconds
#include <Eigen/Dense> // Eigen::VectorXf

namespace Gomoku {

using std::uint8_t;
using std::chrono::milliseconds;
using std::chrono_literals::operator""ms;

inline namespace Config {
enum MinimaxConfig {
    MIN_DEPTH = 2, MAX_DEPTH = 5
};
}

// Minimax树
class Minimax { 
public:
    // Minimax树结点。
    struct Node {
        /*
            树结构部分 - 父结点。
            结点为观察指针，不对其拥有所有权。
        */
        Node* parent = nullptr; 

        /*
            数据部分：
              * position: 当前局面下最后一手下的位置。
              * player:   当前局面下最后一手下的玩家。
              * value:    结点对应局面对于结点对应玩家的价值
        */
        Position position = Position::npos;
        Player   player   = Player::None;
        float    value    = 0.0f;

        /*
            树结构部分 - 子结点。
            结点为独有指针集合，对每个子结点拥有所有权。
        */
        std::vector<std::unique_ptr<Node>> children = {};

        /*
            构造与赋值函数。
            声明一个多参数版本协助完美转发。
            显式声明移动版本，以阻止复制版本的自动生成。
        */
        Node() = default;
        Node(Node*, Position, Player);
        Node(Node&&) = default;
        Node& operator=(Node&&) = default;
    };

    // 置换表的记录。内存被严格控制。
    struct Entry {
        enum NodeType : uint8_t {
            Alpha, Exact, Beta
        };

        float value;   // 4 Byte
        Position pose; // 2 Byte
        NodeType type; // 1 Byte
        uint8_t depth; // 1 Byte
    };

public:
    // 通过深度控制模拟迭代。
	Minimax(
		short    c_depth,
		Position last_move   = -1,
		Player   last_player = Player::White
	);

    Position getAction(Board& board);
    
    // 将Minimax树往深推进一层
    Node* stepForward();              // 选出子结点中的最好手
    Node* stepForward(Position move); // 根据提供的动作往下走
 
    void syncWithBoard(const Board& board);  // 同步搜索树与棋盘，使得根节点为棋盘的最后一手
    void reset(); // 重置minimax树与其所用的策略

private:
    // 按传入的概率扩张子结点并排序
    std::size_t expand(Node* node, Eigen::VectorXf action_probs);

    // 更新根结点，并自动释放剩下的非新树结点
    Node* updateRoot(std::unique_ptr<Node>&& next_root);

    // Alpha-Beta剪枝
    float alphaBeta(Node* node, int depth, float alpha, float beta);

public:
    std::unique_ptr<Node> m_root; //根节点
    std::unordered_map<BoardMap, Entry> m_transTable;
    Evaluator m_evaluator;
    BoardMap& m_boardMap;

private:
    enum class Constraint {
        Depth, Duration
    } c_constraint;
};

}

#endif // !GOMOKU_MCTS_H_