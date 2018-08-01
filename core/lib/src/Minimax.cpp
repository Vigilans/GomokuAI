#include "Minimax.h"
#include "policies/Random.h"
#include "algorithms/Statistical.hpp"
#include "algorithms/Minimax.hpp"
#include "algorithms/Heuristic.hpp"
#include <algorithm>

using namespace std;
using namespace std::chrono;
using Eigen::VectorXf;

namespace Gomoku {

	using Algorithms::Stats;
	using Heuristic = Algorithms::Heuristic;
	using MinimaxAlgorithm = Algorithms::MinimaxAlgorithm;

	/* ------------------- MinimaxPolicy类实现 ------------------- */

	//MinimaxPolicy::MinimaxPolicy(){
	//// MinimaxPolicy::MinimaxPolicy(ExpandFunc f2, EvalFunc f3, double c_depth){
	//    //: expand(f2 ? f2 : [this](auto node, auto& board, auto probs) { 
	//    //    return Minimax::Expand(this, node, board, std::move(probs)); 
	//    //}),
	//    //evaluate(f3 ? f3 : [this](auto& board) { 
	//    //    return Default::Simulate(this, board); 
	//    //}),
	//    //backPropogate(f4 ? f4 : [this](auto node, auto& board, auto value) { 
	//    //    return Default::BackPropogate(this, node, board, value); 
	//    //})
	//}

	// 若未被重写，则创建基类结点
	std::shared_ptr<MinimaxNode> MinimaxPolicy::createNode(MinimaxNode* parent, Position pose, Player player, float cur_value, float fin_value) {
		return std::make_unique<MinimaxNode>(MinimaxNode{ parent, pose, player, cur_value, fin_value });
	}

	void MinimaxPolicy::prepare(Board& board) {
		m_initActs = board.m_moveRecord.size(); // 记录棋盘起始位置
	}

	void MinimaxPolicy::cleanup(Board& board) {
		board.revertMove(board.m_moveRecord.size() - m_initActs); // 保证一定重置回初始局面
	}

	Player MinimaxPolicy::applyMove(Board& board, Position move) {
		return board.applyMove(move, false); // 非终点节点无需检查是否胜利（此前已检查过了）
	}

	Player MinimaxPolicy::revertMove(Board& board, size_t count) {
		return board.revertMove(count);
	}

	bool MinimaxPolicy::checkGameEnd(Board& board) {
		return board.checkGameEnd();
	}

	/* ------------------- Minimax类实现 ------------------- */

	// 更新后，原根节点由shared_ptr自动释放，其余的非子树结点也会被链式自动销毁。
	inline MinimaxNode* updateRoot(Minimax& Minimax, std::shared_ptr<MinimaxNode>&& next_node) {
		Minimax.m_root = std::move(next_node);
		Minimax.m_root->parent = nullptr;
		return Minimax.m_root.get();
	}

	Minimax::Minimax(
		short   c_depth = 12,
		Position last_move ={ Config::GameConfig::WIDTH / 2 ,Config::GameConfig::HEIGHT / 2 } ,
		Player   last_player = Player::Black
	) :
		m_policy(new MinimaxPolicy),
		m_root(m_policy->createNode(nullptr, last_move, last_player, 0.0, 0.0)),
		m_depth(c_depth),
		c_constraint(Constraint::Depth) {
			m_alpha = -0xffff;
			m_beta = 0xffff;
			node_trace.push_back(m_root);
	};

	Position Minimax::getAction(Board& board) {
		float ans = runPlayouts(board);
		return Minimax::stepForward(ans)->position;
	}

	// MinimaxPolicy::EvalResult 
	// 利用value作为minimax计分
	// 在expand过程中利用filter判断各点概率，按概率排序
	// 但每次都需要重新更改alpha,beta的值


	void Minimax::syncWithBoard(Board & board) {

		auto iter = std::find(board.m_moveRecord.begin(), board.m_moveRecord.end(), m_root->position);
		for (iter = (iter == board.m_moveRecord.end() ? board.m_moveRecord.begin() : ++iter);
			iter != board.m_moveRecord.end(); ++iter) {
			stepForward(*iter);
		}
	}

	MinimaxNode* Minimax::stepForward() {
		//if (m_root->player == Player::White) {
		//	auto iter = max_element(m_root->children.begin(), m_root->children.end(), [](auto&& lhs, auto&& rhs) {
		//		return lhs->final_state_value < rhs->final_state_value;
		//	});
		//	return iter != m_root->children.end() ? updateRoot(*this, std::move(*iter)) : m_root.get();
		//}
		//else if (m_root->player == Player::Black) {
			auto iter = max_element(m_root->children.begin(), m_root->children.end(), [](auto&& lhs, auto&& rhs) {
				return lhs->final_state_value > rhs->final_state_value;
			});
			node_trace.push_back(*iter);// add current action into node list
			return iter != m_root->children.end() ? updateRoot(*this, std::move(*iter)) : m_root.get();
		//}
	}

	MinimaxNode* Minimax::stepForward(Position next_move) {
		auto iter = find_if(m_root->children.begin(), m_root->children.end(), [next_move](auto&& node) {
			return node->position == next_move;
		});
		node_trace.push_back(*iter);
		if (iter == m_root->children.end()) { // 这个迷之hack是为了防止Python模块中出现引用Bug...
			iter = m_root->children.emplace(
				m_root->children.end(),
				m_policy->createNode(nullptr, next_move, -m_root->player, 0.0f, 1.0f)
			);
		}
		return updateRoot(*this, std::move(*iter));
	}

	MinimaxNode* Minimax::stepForward(float score) {
		auto iter = find_if(m_root->children.begin(), m_root->children.end(), [score](auto&& node) {
			return node->final_state_value == score;
		});
		//if (iter == m_root->children.end()) { // 这个迷之hack是为了防止Python模块中出现引用Bug...
		//	iter = m_root->children.emplace(
		//		m_root->children.end(),
		//		m_policy->createNode(nullptr, next_move, -m_root->player, 0.0f, 1.0f)
		//	);
		//}
		return updateRoot(*this, std::move(*iter));
	}


	void Minimax::reset() {
		auto iter = m_root->children.emplace(
			m_root->children.end(),
			m_policy->createNode(nullptr, Position(-1), Player::White, 0.0f, 1.0f)
		);
		m_root = std::move(*iter);
	}



	// size_t Minimax::playout(Board& board, Depth = ) {
	//     MinimaxNode* node = m_root.get();      // 裸指针用作观察指针，不对树结点拥有所有权

	//     while (!node->isLeaf()) {   // 检测当前结点是否所有可行手都被拓展过
	//         node = m_MinimaxPolicy->select(node);  // 若当前结点已拓展完毕，则根据价值公式选出下一个探索结点
	//         m_MinimaxPolicy->applyMove(board, node->position);
	//     }

	//     double node_value;
	//     size_t expand_size;

	//     if (!m_MinimaxPolicy->checkGameEnd(board)) {  // 检查终结点游戏是否结束
	//         auto [state_value, action_probs] = m_MinimaxPolicy->simulate(board); // 获取当前盘面相对于「当前应下玩家」的价值与概率分布
	//         expand_size = m_MinimaxPolicy->expand(node, board, std::move(action_probs)); // 根据传入的概率向量扩展一层结点
	//         node_value = -state_value; // 由于node保存的是「下出变成当前局面的一手」的玩家，因此其价值应取相反数
	//     } else {
	//         expand_size = 0;
	//         node_value = CalcScore(node->player, board.m_winner); // 根据绝对价值(winner)获取当前局面于玩家的相对价值
	//     }

	//     m_MinimaxPolicy->backPropogate(node, board, node_value);     
	//     m_MinimaxPolicy->revertMove(board, board.m_moveRecord.size() - m_MinimaxPolicy->m_initActs); // 重置回初始局面
	//     return expand_size;
	// }



	float Minimax::runPlayouts(Board& bd) {
		Board board(bd);
		syncWithBoard(board);
		m_policy->prepare(board);
		//只考虑了depth作为constraints的情况	
		float ans = -0xffff;
		ans = MinimaxAlgorithm::ab_max(m_root, board, m_alpha, m_beta, 0, m_depth);
		m_depth++;
		cout << "m_depth :" << m_depth << endl;
		return ans;
	}
}