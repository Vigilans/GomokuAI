#ifndef GOMOKU_AGENT_H_
#define GOMOKU_AGENT_H_
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include "Game.h"
#include "MCTS.h"
#include "algorithms/Handicraft.h"

namespace Gomoku {

using json = nlohmann::json;
void to_json(json& j, const Position& p) { j = { { "x", p.x() },{ "y", p.y() } }; }
void from_json(const json& j, Position& p) { p = Position{ j["x"], j["y"] }; }

namespace Interface {

class Agent {
public:
    virtual std::string name() = 0;

    virtual Position getAction(Board& board) = 0;

    virtual json debugMessage() { return json(); };

    virtual void syncWithBoard(Board& board) { };

    virtual void reset() { }
};

class HumanAgent : public Agent {
public:
    virtual std::string name() {
        return "HumanAgent";
    }

    virtual Position getAction(Board& board) {
        using namespace std;
        int x, y;
        cout << "\nInput your move: ";
        cin >> hex >> x >> y;
        return { x, y };
    }
};

class RandomAgent : public Agent {
public:
    virtual std::string name() {
        return "RandomAgent";
    }

    virtual Position getAction(Board& board) {
        return board.getRandomMove();
    }
};

class MCTSAgent : public Agent {
public:
    MCTSAgent(milliseconds durations, Policy* policy) : c_duration(durations), m_policy(policy) { }

    virtual std::string name() {
        using namespace std::chrono;
        return "MCTSAgent:" + std::to_string(c_duration.count()) + "ms";
    }

    virtual Position getAction(Board& board) {
        return m_mcts->getAction(board);
    }

    virtual json debugMessage() {
        return {
            { "iterations", m_mcts->m_iterations },
            { "duration",   std::to_string(m_mcts->m_duration.count()) + "ms" }
        };
    };

    virtual void syncWithBoard(Board& board) {
        if (m_mcts == nullptr) {
            auto last_action = board.m_moveRecord.empty() ? Position(-1) : board.m_moveRecord.back();
            m_mcts = std::make_unique<MCTS>(c_duration, last_action, -board.m_curPlayer, m_policy);
        } else {
            m_mcts->syncWithBoard(board);
        }
    };

    virtual void reset() {
        m_mcts->reset();
    }

protected:
    std::unique_ptr<MCTS> m_mcts;
    std::shared_ptr<Policy> m_policy;
    std::chrono::milliseconds c_duration;
};

class PatternEvalAgent : public Agent {
public:
    virtual std::string name() {
        return "PatternEvalAgent";
    }

    virtual Position getAction(Board& board) {
        using Gomoku::Algorithms::Handicraft;
        auto init_player = board.m_curPlayer;
        Eigen::VectorXf action_probs;

        // Pre-evaluate the board using handicraft policy
        auto self_scores = Handicraft::EvalScores(m_evaluator, init_player);
        auto rival_scores = Handicraft::EvalScores(m_evaluator, -init_player);

        Position next_move;

        auto[decisive_moves, is_antiMove] = Handicraft::CheckDecisive(m_evaluator, board);
        if (!decisive_moves.empty()) {
            action_probs = Eigen::VectorXf::Zero(BOARD_SIZE);
            for (auto move : decisive_moves) {
                action_probs[move] = 1.0f / decisive_moves.size();
            }
            next_move = decisive_moves[rand() % decisive_moves.size()];
        }
        else {
            action_probs = Handicraft::ScoreBasedProbs(m_evaluator, board, self_scores, rival_scores);
            action_probs.maxCoeff(&next_move.id);
        }

        return next_move;
    }

    virtual json debugMessage() {
        return json();
    };

    virtual void syncWithBoard(Board& board) {
        m_evaluator.syncWithBoard(board);
    };

    virtual void reset() {
        m_evaluator.reset();
    }

private:
    Handicrafts::Evaluator m_evaluator;
};

}

}

#endif // !GOMOKU_AGENT_H_