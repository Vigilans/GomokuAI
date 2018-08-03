#include "pch.h"

using namespace std::chrono;
using namespace Gomoku::Interface;
using namespace Gomoku::Policies;

int main() {

    HumanAgent agent1;
    RandomAgent agent2;
    MCTSAgent agent3(1s, new RandomPolicy);
    //MCTSAgent agent4(10000, new RandomPolicy);
    MCTSAgent agent6(2000ms, new TraditionalPolicy(5));
    MCTSAgent agent6x(1001ms, new TraditionalPolicy(7));
    PatternEvalAgent agent7;
	MinimaxAgent agent8(3);
    //MCTSAgent agent7x(50000, new PoolRAVEPolicy(2, 0));

#ifndef _DEBUG
    return NewConsoleInterface(agent1, agent6, true);
#else
	return NewConsoleInterface(agent1, agent1, false);
#endif
    //return KeepAliveBotzoneInterface(agent6);
    //return BotzoneInterface(agent6);
}