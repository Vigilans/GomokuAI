from .bin import module_path as __origin__  # Add proper CorePyExt's path to sys path
from CorePyExt import *

del bin  # Clear the intermediary module
__doc__ = "C++ extension 'core' with origin path at '{}'".format(__origin__)
