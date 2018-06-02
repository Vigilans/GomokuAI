from config import MCTS_CONFIG
from core import Policy
from .mcts import MCTSAgent


def PyConvNetAgent(network, c_puct, **constraint):
    return MCTSAgent(
        policy=Policy(eval_state=network.eval_state, c_puct=c_puct),
        **constraint
    )


def AlphaZeroAgent(**constraint):
    pass


if __name__ == "__main__":
    from .utils import botzone_interface
    from network import PolicyValueNetwork
    model_file = "path/to/model/file"
    botzone_interface(PyConvNetAgent(PolicyValueNetwork(model_file)))
