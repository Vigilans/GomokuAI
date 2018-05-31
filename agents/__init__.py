"""
Expose inner modules for easily import
"""
from .agent import Agent, Agent as RandomAgent
from .human import ConsoleAgent
from .botzone import BotzoneAgent
from .mcts import MCTSAgent, MCTSAgent as RandomMCTSAgent
from .mcts import RAVEAgent, TraditionalAgent
from .alphazero import AlphaZeroAgent, PyConvNetAgent


AGENT_MAP = {
    "random": RandomAgent,
    "console": ConsoleAgent,
    "botzone": BotzoneAgent,
    "random_mcts": RandomMCTSAgent,
    "rave": RAVEAgent,
    "traditional": TraditionalAgent,
    "alphazero": AlphaZeroAgent
}
