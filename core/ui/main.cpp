#include "Agent.h"
#include "Interface.h"

using namespace Gomoku::Interface;
using namespace Gomoku::Policies;

int main() {
    HumanAgent agent1;
    RandomAgent agent2;
    MCTSAgent agent3(50000, new RandomPolicy);
    MCTSAgent agent4(10000, new RandomPolicy);
    MCTSAgent agent5(5000, new TraditionalPolicy(1e-4, 1e-1));
    MCTSAgent agent6(5001, new TraditionalPolicy(5, 0));
    return ConsoleInterface(agent6, agent3);
}