cmake_minimum_required (VERSION 3.6)

project (CoreLib)

include_directories(./include)
include_directories(./lib)

add_library (CoreLib, STATIC,
    src/Game.cpp,
    src/MCTS.cpp
    src/Evaluator.cpp
)
