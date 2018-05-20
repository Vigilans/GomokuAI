""" Add root folder to top-level """
import sys
import os
root_path = os.path.realpath(
    os.path.dirname(__file__) + "/../"
)
if root_path not in sys.path:
    sys.path.insert(0, root_path)
del sys, os, root_path

__package__ = "network"  # fix package name

from .model_tf import PolicyValueNetwork
# from .model_keras import PolicyValueNetwork
from .data_helper import DataHelper
from .train import TrainingPipeline
