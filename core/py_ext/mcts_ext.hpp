#include "pch.h"
#include "lib/include/MCTS.h"

using namespace Gomoku;
using namespace std;

inline void MCTS_Ext(py::module& mod) {
    // Expose the core lib namespace
    using namespace Gomoku;

    py::class_<Node>(mod, "Node", "MCTS Tree Node")
        .def(py::init<Node*, Position, Player, float, float>(),
            py::arg("parent") = nullptr,
            py::arg("position") = Position(-1),
            py::arg("player") = Player::None,
            py::arg("state_value") = 0.0,
            py::arg("action_prob") = 0.0
        )
        .def_readonly("parent", &Node::parent)
        .def_readonly("position", &Node::position)
        .def_readonly("player", &Node::player)
        .def_readwrite("state_value", &Node::state_value)
        .def_readwrite("action_prob", &Node::action_prob)
        .def_readwrite("node_visits", &Node::node_visits)
        .def_property_readonly("children", [](const Node* n) {   
            py::list children(n->children.size());
            for (int i = 0; i < children.size(); ++i) {  // py::list do not support writing by iterator
                children[i] = n->children[i].get();                
            }
            return children;
        })
        .def("is_leaf", &Node::isLeaf)
        .def("is_full", &Node::isFull)
        .def("__repr__", [](const Node* n) { 
            return py::str("Node(pose: {}, player: {}, value: {}, prob: {}, visits: {}, childs: {})").format(
                n->position, n->player, n->state_value, n->action_prob, n->node_visits, n->children.size()
            ); 
        });


    py::class_<Policy>(mod, "Policy", "MCTS Tree Policy")
        .def(py::init<Policy::SelectFunc, Policy::ExpandFunc, Policy::EvalFunc, Policy::UpdateFunc>(),
            py::arg("select") = nullptr,
            py::arg("expand") = nullptr,
            py::arg("eval_state") = nullptr,
            py::arg("back_prop") = nullptr
        )
        .def_readonly("select", &Policy::select)
        .def_readonly("expand", &Policy::expand)
        .def_readonly("eval_state", &Policy::simulate)
        .def_readonly("back_prop", &Policy::backPropogate);


    py::class_<MCTS>(mod, "MCTS", "Monte Carlo Tree Search")
        .def(py::init<Position, Player, Policy*, size_t, double>(),
            py::arg("last_move") = Position(-1),
            py::arg("last_player") = Player::White,
            py::arg_v("policy", nullptr, "Default Policy"),
            py::arg("c_iterations") = C_ITERATIONS,
            py::arg("c_puct") = sqrt(2)
        )
        .def_readonly("size", &MCTS::m_size)
        .def_readonly("c_puct", &MCTS::c_puct)
        .def_readonly("c_iterations", &MCTS::c_iterations)
        .def_property_readonly("root", [](const MCTS& m) { return m.m_root.get(); })
        .def_property_readonly("policy", [](const MCTS& m) { return m.m_policy.get(); })
        .def("eval_state", &MCTS::evalState)
        .def("step_forward", [](MCTS& m) { m.stepForward(); }) // Return value couldn't be exposed since it may get GC. 
        .def("step_forward", [](MCTS& m, Position p) { m.stepForward(p); }, py::arg("next_move"))
        .def("get_action",   &MCTS::getNextMove)
        .def("reset", &MCTS::reset)
        .def("__repr__", [](const MCTS& m) { return py::str("MCTS(root_player: {}, nodes: {})").format(m.m_root->player, m.m_size); });
}