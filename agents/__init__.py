""" Add root folder to top-level """
import sys
import os
root_path = os.path.realpath(
    os.path.dirname(__file__) + "/../"
)
if root_path not in sys.path:
    sys.path.insert(0, root_path)
del sys, os, root_path

__package__ = "agents"  # fix package name

"""
Expose inner modules for easily import
"""
from . import utils
from .agent import Agent, Agent as RandomAgent
from .mcts_agent import MCTSAgent, MCTSAgent as DefaultMCTSAgent
