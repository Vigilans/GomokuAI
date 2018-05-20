if __name__ == "__main__":
    from __init__ import MCTSAgent
else:
    from .mcts_agent import MCTSAgent

from core import Policy


class AlphaZeroAgent(MCTSAgent):
    """ Powerful Agent using MCTS + RCNN """
    def __init__(self, network, self_play=False):
        self.network = network
        super().__init__(
            Policy(eval_state=self.network.eval_state), self_play
        )

    def __str__(self):
        return "AlphaZero " + super().__str__(self)


if __name__ == "__main__":
    from utils import keepalive_botzone_interface
    from network import PolicyValueNetwork
    keepalive_botzone_interface(AlphaZeroAgent(PolicyValueNetwork()))
