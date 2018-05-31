#ifndef GOMOKU_ALGORITHMS_RAVE_H_
#define GOMOKU_ALGORITHMS_RAVE_H_
#include "Default.h"

namespace Gomoku::Algorithms {

struct AMAFNode : public Node {

    // AMAF算法的相关价值属性。
    float amaf_value = 0.0;
    size_t amaf_visits = 0;

    // 目前MSVC(Visual Studio 2017 15.6)仍未支持Extended aggregate initialization，故需手动写一个构造函数
    AMAFNode(Node* parent = nullptr, Position pose = -1, Player player = Player::None, float Q = .0f, float P = .0f, float amaf_Q = .0f, size_t amaf_N = 0)
        : Node{ parent, pose, player, Q, P }, amaf_value(amaf_Q), amaf_visits(amaf_N) { }
};

struct RAVE {

    static double HandSelect(const AMAFNode* node, size_t eqv_param = 800) {
        double n = node->node_visits;
        double k = eqv_param;
        return sqrt(k / (3*n + k));
    }

    static double MinMSE(const AMAFNode* node, double c_bias) {
        // Schedule that minimises Mean Squared Error
        double n = node->node_visits + 1;
        double n_ = node->amaf_visits;
        return n_ / (n + n_ + n*n_*c_bias*c_bias); // σ^2 = 1-Q^2
    }

    static double WeightedValue(const AMAFNode* node, double c_bias) {
        /*double weight = MinMSE(node, c_bias);*/
        double weight = HandSelect(node);
        return (1 - weight) * node->state_value + weight * node->amaf_value;
    }

    /*
        RAVE算法并不负责MCTS的Expand与使用Default Policy Simulate的过程，因而不提供相关函数。
    */

    static Node* Select(Policy* policy, const Node* node) {
        // 由于BackPropogate阶段已作过调整，只需取第一个值即可。
        return node->children[0].get();
    }

    // 反向传播更新结点价值，要求传入的Board处于游戏结束的状态。
    static void BackPropogate(Policy* policy, Node* node, Board& board, float value, double c_bias, bool useRave = true) {
        for (; node != nullptr; node = node->parent, value = -value) {
            size_t max_index = 0;
            double max_score = -INFINITY;
            // 更新子结点的RAVE价值，顺便计算最终得分
            for (int i = 0; i < node->children.size(); ++i) {
                auto rave_node = static_cast<AMAFNode*>(node->children[i].get());
                if (useRave && board.moveState(rave_node->player, rave_node->position)) { // 要求是同一玩家下的
                    rave_node->amaf_visits += 1;
                    rave_node->amaf_value += (-value - rave_node->amaf_value) / rave_node->amaf_visits;
                }
                auto score = useRave ? WeightedValue(rave_node, c_bias) : rave_node->state_value + Default::PUCB(rave_node, policy->c_puct);
                if (score > max_score) {
                    max_score = score, max_index = i;
                }
            }
            if (!node->children.empty()) {
                node->children[0].swap(node->children[max_index]); // 得分最大的子结点提升至容器首位
            }    
            node->node_visits += 1;
            node->state_value += (value - node->state_value) / node->node_visits;
        }
    }
};

}

#endif // !GOMOKU_ALGORITHMS_RAVE_H_