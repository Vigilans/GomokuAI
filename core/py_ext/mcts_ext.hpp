#include "pch.h"
#include "lib/include/MCTS.h"

using namespace Gomoku;
using namespace std;

inline void MCTS_Ext(py::module& mod) {
    // Expose the core lib namespace
    using namespace Gomoku;

    py::class_<Node>(mod, "Node", "MCTS Tree Node")
        .def(py::init<Node*, Position, Player, double, double>(),
            py::arg("parent") = nullptr,
            py::arg("position") = Position(-1),
            py::arg("player") = Player::None,
            py::arg("state_value") = 0.0,
            py::arg("action_prob") = 0.0
        )
        .def_readwrite("parent", &Node::parent)
        .def_readwrite("position", &Node::position)
        .def_readwrite("player", &Node::player)
        .def_readwrite("state_value", &Node::state_value)
        .def_readwrite("action_prob", &Node::action_prob)
        .def_readwrite("node_visits", &Node::node_visits)
        .def("is_leaf", &Node::isLeaf)
        .def("is_full", &Node::isFull);


    py::class_<Policy>(mod, "Policy", "MCTS Tree Policy")
        .def(py::init<Policy::SelectFunc, Policy::ExpandFunc, Policy::EvalFunc, Policy::UpdateFunc>(),
            py::arg("select") = nullptr,
            py::arg("expand") = nullptr,
            py::arg("evaluate") = nullptr,
            py::arg("update") = nullptr
        )
        .def_readonly("select", &Policy::select)
        .def_readonly("expand", &Policy::expand)
        .def_readonly("evaluate", &Policy::simulate)
        .def_readonly("update", &Policy::backPropogate);


    py::class_<MCTS>(mod, "MCTS", "Monte Carlo Tree Search")
        .def(py::init<Position, Player, Policy*, size_t, double>(),
            py::arg("last_move") = Position(-1),
            py::arg("last_player") = Player::White,
            py::arg_v("policy", nullptr, "Default Policy"),
            py::arg("c_iterations") = 20000,
            py::arg("c_puct") = sqrt(2)
        )
        .def_readonly("size", &MCTS::m_size)
        .def_readonly("c_puct", &MCTS::c_puct)
        .def_readonly("c_iterations", &MCTS::c_iterations)
        .def_property_readonly("root", [](const MCTS& mcts) { return mcts.m_root.get(); })
        .def_property_readonly("policy", [](const MCTS& mcts) { return mcts.m_policy.get(); })
        .def("eval_state", &MCTS::evalState)
        .def("step_forward", static_cast<Node* (MCTS::*)()>(&MCTS::stepForward))
        .def("step_forward", static_cast<Node* (MCTS::*)(Position)>(&MCTS::stepForward), py::arg("next_move"))
        .def("get_action",   &MCTS::getNextMove);
}