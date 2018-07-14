#include "pch.h"

using namespace std::chrono;
using namespace Gomoku::Interface;
using namespace Gomoku::Policies;

int main() {
    HumanAgent agent1;
    //RandomAgent agent2;
    MCTSAgent agent3(1s, new RandomPolicy);
    //MCTSAgent agent4(10000, new RandomPolicy);
    MCTSAgent agent6(2000ms, new TraditionalPolicy(5));
    MCTSAgent agent6x(901ms, new TraditionalPolicy(5));
    PatternEvalAgent agent7;
    //MCTSAgent agent7x(50000, new PoolRAVEPolicy(2, 0));

    return ConsoleInterface(agent7, agent6x);
    //return KeepAliveBotzoneInterface(agent6x);
    //return BotzoneInterface(agent6);
}