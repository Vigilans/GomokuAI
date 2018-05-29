#include "pch.h"
#include "lib/include/MCTS.h"

//using namespace Gomoku;
//using namespace std;

inline void MCTS_Ext(py::module& mod) {
    // Expose the core lib namespace
    using namespace Gomoku;
    using namespace std;

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


    // Register Policy class with shared_ptr holder type
    py::class_<Policy, std::shared_ptr<Policy>>(mod, "Policy", "MCTS Tree Policy")
        .def(py::init<Policy::SelectFunc, Policy::ExpandFunc, Policy::EvalFunc, Policy::UpdateFunc, double>(),
            py::arg("select") = nullptr,
            py::arg("expand") = nullptr,
            py::arg("eval_state") = nullptr,
            py::arg("back_prop") = nullptr,
            py::arg("c_puct") = C_PUCT
        )
        .def("prepare", &Policy::prepare)
        .def("clean_up", &Policy::cleanup)
        .def("apply_move", &Policy::applyMove)
        .def("revert_move", &Policy::revertMove)
        .def("check_game_end", &Policy::checkGameEnd)
        .def("create_node", &Policy::createNode)
        .def_readonly("select", &Policy::select)
        .def_readonly("expand", &Policy::expand)
        .def_readonly("eval_state", &Policy::simulate)
        .def_readonly("back_prop", &Policy::backPropogate)
        .def("__repr__", [](const Policy& p) { return py::str("Policy(c_puct: {}, init_acts: {})").format(p.c_puct, p.m_initActs); });


    py::class_<MCTS>(mod, "MCTS", "Monte Carlo Tree Search")
        .def(py::init<milliseconds, Position, Player, shared_ptr<Policy>>(),
            py::arg("c_duration") = 960ms,
            py::arg("last_move") = Position(-1),
            py::arg("last_player") = Player::White,
            py::arg_v("policy", nullptr, "Default Policy")
        )
        .def(py::init<size_t, Position, Player, shared_ptr<Policy>>(),
            py::arg("c_iterations"),
            py::arg("last_move") = Position(-1),
            py::arg("last_player") = Player::White,
            py::arg_v("policy", nullptr, "Default Policy")
        )
        .def_readonly("size", &MCTS::m_size)
        .def_readonly("iterations", &MCTS::m_iterations)
        .def_readonly("duration", &MCTS::m_duration)
        .def_property_readonly("root", [](const MCTS& m) { return m.m_root.get(); })
        .def_property_readonly("policy", [](const MCTS& m) { return m.m_policy.get(); })
        .def("get_action", &MCTS::getAction)
        .def("eval_state", &MCTS::evalState)
        .def("step_forward", [](MCTS& m) { m.stepForward(); }) // Return value couldn't be exposed since it may get GC. 
        .def("step_forward", [](MCTS& m, Position p) { m.stepForward(p); }, py::arg("next_move"))
        .def("sync_with_board", &MCTS::syncWithBoard)
        .def("reset", &MCTS::reset)
        .def("__repr__", [](const MCTS& m) { return py::str("MCTS(root_player: {}, nodes: {})").format(m.m_root->player, m.m_size); });
}