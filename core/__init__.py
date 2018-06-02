from .bin import module_path as __origin__  # Add proper CorePyExt's path to sys path
from CorePyExt import GameConfig, Player, Position, Board
from CorePyExt import Node, Policy, MCTS
from CorePyExt import RandomPolicy, PoolRAVEPolicy, TraditionalPolicy

del bin  # Clear the intermediary module
__doc__ = f"C++ extension 'core' with origin path at '{__origin__}'"
