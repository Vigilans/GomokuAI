#include "Agent.h"
#include "Interface.h"
#include "policies/Random.h"
#include "policies/PoolRAVE.h"
#include "policies/Traditional.h"

using namespace std::chrono;
using namespace Gomoku::Interface;
using namespace Gomoku::Policies;

int main() {
    HumanAgent agent1;
    //RandomAgent agent2;
    //MCTSAgent agent3(100000, new RandomPolicy);
    //MCTSAgent agent4(10000, new RandomPolicy);
    MCTSAgent agent6(950ms, new TraditionalPolicy(3, 1e-4, true));
    MCTSAgent agent6x(960ms, new TraditionalPolicy(5, 0, false));
    //MCTSAgent agent7x(50000, new PoolRAVEPolicy(2, 0));

    //return ConsoleInterface(agent6, agent6x);
    //return KeepAliveBotzoneInterface(agent6x);
    return BotzoneInterface(agent6x);
}
