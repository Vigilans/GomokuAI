#include "pch.h"
#include "lib/include/Game.h"

inline void Game_Ext(py::module& mod) {
    // Import the `_a` literal
    using namespace py::literals;

    // Expose the core lib namespace
    using namespace Gomoku;


    // Definition of Game Config enum
    //py::class_<GameConfig>(mod, "Config", "Gomoku game configs")
    //    .def_readonly_static("width", GameConfig::WIDTH)
    //    .def_readonly_static("height", &GameConfig::HEIGHT)
    //    .def_readonly_static("board_size", &GameConfig::BOARD_SIZE)
    //    .def_readonly_static("max_renju", &GameConfig::MAX_RENJU);


    // Definition of Player enum class
    py::enum_<Player>(mod, "Player", "Gomoku player types")
        .value("white", Player::White)
        .value("none",  Player::None)
        .value("black", Player::Black)
        .def(py::init([](int p) { return Player(p); }))
        .def("__float__", [](Player p) { return static_cast<double>(p); })
        .def("__neg__",   [](Player p) { return -p; })
        .def_static("calc_score", [](Player p, Player w) { return getFinalScore(p, w); });


    // Definition of Position struct
    py::class_<Position>(mod, "Position", "Gomoku board positions")
        .def(py::init<Position::value_type>())                       // id
        .def(py::init<Position::value_type, Position::value_type>()) // (x, y)
        .def_readwrite("id", &Position::id)
        .def_property("x", &Position::x, [](Position& p, Position::value_type x) { p.id = p.y() * WIDTH + x; })
        .def_property("y", &Position::y, [](Position& p, Position::value_type y) { p.id += (y - p.y()) * WIDTH; })
        .def("__int__",  [](const Position& p) { return p.id; })
        .def("__hash__", [](const Position& p) { return std::hash<Position>()(p); })
        .def("__repr__", [](const Position& p) { return py::str("Position {}").format(std::to_string(p)); })
        .def("__str__",  [](const Position& p) { return std::to_string(p); });


    // Definition of Board class
    py::class_<Board>(mod, "Board", "Gomoku game board")
        .def(py::init<>())
        .def("apply_move",  &Board::applyMove, py::arg("move"), py::arg("checkVictory") = true)
        .def("revert_move", &Board::revertMove)
        .def("random_move", &Board::getRandomMove)
        .def("check_move",  &Board::checkMove)
        .def("check_end",   &Board::checkGameEnd)
        .def_property_readonly("status", [](const Board& b) {
            auto status = b.status();
            return py::dict(
                "is_end"_a = status.end,
                "cur_player"_a = status.curPlayer,
                "winner"_a = status.winner
            );
        })
        .def_property_readonly("move_counts", [](const Board& b) {
            py::dict move_counts;
            for (int i = 0; i < 3; ++i) {
                const auto player = Player(i - 1);
                move_counts[py::cast(player)] = b.moveCounts(player);
            }
            return move_counts;
        })
        .def_property_readonly("move_states", [](const Board& b) {
            py::dict move_states;
            for (int i = 0; i < 3; ++i) {
                const auto player = Player(i - 1);
                // reshape to a square board
                move_states[py::cast(player)] = py::array(py::dtype("uint8"), { WIDTH, HEIGHT }, &b.moveStates(player)[0]); 
            }
            return move_states;
        })
        .def("__repr__", [](const Board& b) { return py::str("Gomoku board with currently {} to move").format(std::to_string(b.m_curPlayer)); })
        .def("__str__",  [](const Board& b) { return std::to_string(b); });
}

