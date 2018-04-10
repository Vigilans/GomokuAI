if __name__ == "__main__":
    from __init__ import Agent
else:
    from .agent import Agent

from core import MCTS
from config import mcts_config as config


class MCTSAgent(Agent):
    def __init__(self, policy=None, self_play=False):
        self.mcts = MCTS(
            policy=policy,
            c_iterations=config["c_iterations"],
            c_puct=config["c_puct"]
        )
        self.self_play = self_play

    def get_action(self, state):
        self._check_root(state)
        return self.mcts.get_action(state)

    def eval_state(self, state):
        self._check_root(state)
        Q, pi = self.mcts.eval_state(state)
        return Q, pi, self.mcts.step_forward().position

    def _check_root(self, state):
        """ ensure root node to be last player's move when not self-playing"""
        cur_player = state.status["cur_player"]
        root_player = self.mcts.root.player
        if not self.self_play and root_player != -cur_player:
            self.mcts.step_forward(state.last_move)

    def __str__(self):
        return "MCTS Agent with {} playouts".format(self.mcts.c_iterations)


if __name__ == "__main__":
    from utils import botzone_interact
    botzone_interact(MCTSAgent())
