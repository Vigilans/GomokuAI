#include "pch.h"
#include "lib/include/MCTS.h"

inline void MCTS_Ext(py::module& mod) {
    using namespace Gomoku;


    py::class_<Node>(mod, "Node", "MCTS node");

}