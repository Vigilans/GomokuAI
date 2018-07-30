#ifndef GOMOKU_ALGORITHMS_MINIMAX_H_
#define GOMOKU_ALGORITHMS_MINIMAX_H_
#include "../Pattern.h"
#include "Statistical.hpp"
#include "../Minimax.h"
#include <algorithm>
#include <tuple>
#include <deque>
#include <utility>
#include <iostream>
using namespace std;
#define DEPTH 4

inline bool action_probs_cmp(pair<float, int> a, pair<float,int> b){
    return a.first > b.first;
}
inline double max(double a, double b){
    return a>b?a:b;
}
inline double min(double a, double b){
    return a>b?b:a;
}

namespace Gomoku::Algorithms {

	struct MinimaxAlgorithm {
		//一次性扩展所有子节点 
		static size_t Expand( std::unique_ptr<MinimaxNode>& node, Board& board, const Eigen::VectorXf action_probs, bool extraCheck = true) {
			if (node->children.size()) return node->children.size();
			//node->children.reserve(action_probs.size());
			vector<pair<float, int>> sort_tmp;
			for (int i = 0; i < BOARD_SIZE; i++) {
				sort_tmp.push_back(pair<float, int>(action_probs[i], i));
			}
			//sort_tmp[i].first : action_probs
			//sort_tmp[i].second : position
			sort(sort_tmp.begin(), sort_tmp.end(), action_probs_cmp); //按照概率降序排序
			for (int i = 0; i < BOARD_SIZE; ++i) { //建树
				if (sort_tmp[i].first == 0.0) break; //之后概率必定都为0
				if (!extraCheck || board.checkMove(sort_tmp[i].second))
					node->children.push_back(MinimaxPolicy::createNode(node.get(), sort_tmp[i].second, -node->player, 0.0f, sort_tmp[i].first));
			}
			return node->children.size();
		}

		static double ab_max(std::unique_ptr<MinimaxNode>& node, Board& board, double &a, double &b, short depth) {
			if (!node->action_probs.size()) { //若size!=0 说明进行过evalstate，也就进行过Expand; 若size == 0，进行evaluate和拓展
				node->action_probs = Minimax::evalState(node, board);
				MinimaxAlgorithm::Expand(node, board, node->action_probs);
			}
			if (node->isLeaf() || depth == DEPTH) return node->current_state_value;

			double /*alpha = a,*/ val = -0xffff;
			for (int i = 0; i < node->children.size(); i++) {
				//更新board
				board.applyMove(node->children[i]->position);
				val = max(val, ab_min(node->children[i], board, a, b, depth + 1));
				board.revertMove();
				if (val >= b) {
					node->final_state_value = val;
					return val;
				}
				a = max(val, a);
			}
			node->final_state_value = val;
			return val;
		}

		static double ab_min(std::unique_ptr<MinimaxNode>& node, Board& board, double &a, double &b, short depth) {
			if (!node->action_probs.size()) { //若size!=0 说明进行过evalstate，也就进行过Expand; 若size == 0，进行evaluate和拓展
				node->action_probs = Minimax::evalState(node, board);
				MinimaxAlgorithm::Expand(node, board, node->action_probs);
			}
			if (node->isLeaf() || depth == DEPTH) return node->current_state_value;

			double /*beta = b, */val = 0xffff;
			for (int i = 0; i < node->children.size(); i++) {
				//更新board
				board.applyMove(node->children[i]->position);
				val = min(val, ab_max(node->children[i], board, a, b, depth + 1));
				board.revertMove();
				if (val >= b) {
					node->final_state_value = val;
					return val;
				}
				b = min(val, b);
			}
			node->final_state_value = val;
			return val;
		}

	};
}

#endif // !GOMOKU_ALGORITHMS_HEURISTIC_H_