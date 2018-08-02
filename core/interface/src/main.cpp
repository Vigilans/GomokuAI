#include "pch.h"
#include <iostream>
#include "perceptron.hpp"
using namespace std;

using namespace std::chrono;
using namespace Gomoku::Interface;
using namespace Gomoku::Policies;


int main() {

    HumanAgent agent1;
    RandomAgent agent2;
    MCTSAgent agent3(1s, new RandomPolicy);
    //MCTSAgent agent4(10000, new RandomPolicy);
    MCTSAgent agent6(1000ms, new TraditionalPolicy(5));
    PatternEvalAgent agent7;
    //MCTSAgent agent7x(50000, new PoolRAVEPolicy(2, 0));
	vector<int> tmp;
	perceptron m_perceptron(tmp);

	int cnt = 1;
	while (cnt--) {
		MCTSAgent agent6x(1001ms, new TraditionalPolicy(7));
		MinimaxAgent agent8;
        m_perceptron.check_valid();
		ConsoleInterface(agent6x, agent8);
		for (auto it = agent8.m_minimax.node_trace.begin(); it != agent8.m_minimax.node_trace.end(); it++) {
			cout << "postion: " << (*it)->position / 15 << "," << (*it)->position % 15 << "score = " << (*it)->current_state_value << endl;
		}
        

	}
    //return KeepAliveBotzoneInterface(agent6);
    //return BotzoneInterface(agent6);
}