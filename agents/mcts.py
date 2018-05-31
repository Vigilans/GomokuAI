from .agent import Agent
from core import MCTS, PoolRAVEPolicy, TraditionalPolicy


class MCTSAgent(Agent):
    """
    Agent Based on Monte Carlo Tree Search.
    Use "c_iterations" or "c_duration" as constraint.
    """
    def __init__(self, policy=None, **constraint):
        self.mcts = MCTS(policy=policy, **constraint)

    def get_action(self, state):
        self.mcts.sync_with_board(state)
        return self.mcts.get_action(state)

    def eval_state(self, state):
        self.mcts.sync_with_board(state)
        Q, pi = self.mcts.eval_state(state)
        self.mcts.step_forward()
        return Q, pi, self.mcts.root.position

    def reset(self):
        self.mcts.reset()

    def __repr__(self):
        return "MCTS Agent with {}".format(self.mcts.policy.__class__.__name__)


def RAVEAgent(c_puct, c_bias, **constraint):
    return MCTSAgent(
        policy=PoolRAVEPolicy(c_puct, c_bias),
        **constraint
    )


def TraditionalAgent(c_puct, c_bias=0.0, use_rave=False, **constraint):
    return MCTSAgent(
        policy=TraditionalPolicy(c_puct, c_bias, use_rave),
        **constraint
    )


if __name__ == "__main__":
    from .utils import botzone_interface
    from config import MCTS_CONFIG as config
    botzone_interface(TraditionalAgent(
        config["c_puct"], c_duration=config["c_duration"]
    ))
