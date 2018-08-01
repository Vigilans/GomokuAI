#include "pch.h"

using namespace std::chrono;
using namespace Gomoku::Interface;
using namespace Gomoku::Policies;

int main() {
    HumanAgent agent1;
    //RandomAgent agent2;
    MCTSAgent agent3(1s, new RandomPolicy);
    //MCTSAgent agent4(10000, new RandomPolicy);
    MCTSAgent agent6(1000ms, new TraditionalPolicy(5));
    PatternEvalAgent agent7;
    //MCTSAgent agent7x(50000, new PoolRAVEPolicy(2, 0));
	int cnt = 1;
	while (cnt--) {
		MCTSAgent agent6x(1001ms, new TraditionalPolicy(7));
		MinimaxAgent agent8;

		ConsoleInterface(agent6x, agent8);

	}
    //return KeepAliveBotzoneInterface(agent6);
    //return BotzoneInterface(agent6);
}