#ifndef GOMOKU_AGENT_H_
#define GOMOKU_AGENT_H_
#include <iostream>
#include <string>
#include "Game.h"
#include "MCTS.h"
#include "policies/Random.h"
#include "policies/Traditional.h"

namespace Gomoku::Interface {

class Agent {
public:
    virtual std::string name() = 0;

    virtual Position getAction(Board& board) = 0;

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
        return { x - 1, y - 1 };
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
    MCTSAgent(size_t iterations, Policy* policy) : c_iterations(iterations), m_policy(policy) { }

    virtual std::string name() {
        using namespace std;
        return "MCTSAgent"s + to_string(c_iterations);
    }

    virtual Position getAction(Board& board) {
        return m_mcts->getNextMove(board);
    }

    virtual void syncWithBoard(Board& board) {
        if (m_mcts == nullptr) {
            m_mcts = std::make_unique<MCTS>(board.m_moveRecord.back(), -board.m_curPlayer, c_iterations, m_policy);
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
    std::size_t c_iterations;
};

}

#endif // !GOMOKU_AGENT_H_