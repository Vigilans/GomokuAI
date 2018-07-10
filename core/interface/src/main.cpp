#include "Agent.h"
#include "Interface.h"
#include "policies/Random.h"
#include "policies/PoolRAVE.h"
#include "policies/Traditional.h"
#include <deque>

using namespace std::chrono;
using namespace Gomoku::Interface;
using namespace Gomoku::Policies;

int main() {
    HumanAgent agent1;
    //RandomAgent agent2;
    //MCTSAgent agent3(100000, new RandomPolicy);
    //MCTSAgent agent4(10000, new RandomPolicy);
    MCTSAgent agent6(900ms, new TraditionalPolicy(5));
    MCTSAgent agent6x(901ms, new TraditionalPolicy(5));
    PatternEvalAgent agent7;
    //MCTSAgent agent7x(50000, new PoolRAVEPolicy(2, 0));

    return ConsoleInterface(agent1, agent7);
    //return KeepAliveBotzoneInterface(agent6x);
    //return BotzoneInterface(agent6);
}