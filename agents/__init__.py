from .agent import Agent, Agent as RandomAgent
from .human import ConsoleAgent
from .botzone import BotzoneAgent
from .mcts import MCTSAgent, RandomMCTSAgent
from .mcts import RAVEAgent, TraditionalAgent
from .alphazero import AlphaZeroAgent, PyConvNetAgent


AGENT_MAP = {
    "random": RandomAgent,
    "console": ConsoleAgent,
    "botzone": BotzoneAgent,
    "mcts": MCTSAgent,
    "random_mcts": RandomMCTSAgent,
    "rave_mcts": RAVEAgent,
    "traditional_mcts": TraditionalAgent,
    "alphazero": PyConvNetAgent
}
