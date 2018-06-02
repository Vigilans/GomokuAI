from core import Policy
from .mcts import MCTSAgent


def PyConvNetAgent(network, c_puct, **constraint):
    return MCTSAgent(
        policy=Policy(eval_state=network.eval_state, c_puct=c_puct),
        **constraint
    )


def AlphaZeroAgent(**constraint):
    pass


def main():
    from .utils import botzone_interface
    from config import MCTS_CONFIG
    from network import PolicyValueNetwork
    model_file = "path/to/model/file"
    network = PolicyValueNetwork(model_file)
    botzone_interface(PyConvNetAgent(network, **MCTS_CONFIG))


if __name__ == "__main__":
    main()
