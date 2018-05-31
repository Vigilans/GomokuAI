#include "pch.h"
#include "lib/include/Game.h"

inline void Game_Ext(py::module& mod) {
    // Import the `_a` literal
    using namespace py::literals;

    // Expose the core lib namespace
    using namespace Gomoku;


    // Definition of Game Config enum
    auto game_config = py::dict(
            "width"_a      = (int)GameConfig::WIDTH,
            "height"_a     = (int)GameConfig::HEIGHT,
            "board_size"_a = (int)GameConfig::BOARD_SIZE,
            "max_renju"_a  = (int)GameConfig::MAX_RENJU
    );
    mod.add_object("GameConfig", game_config);


    // Definition of Player enum class
    py::enum_<Player>(mod, "Player", "Gomoku player types")
        .value("white", Player::White)
        .value("none",  Player::None)
        .value("black", Player::Black)
        .def("__float__", [](Player p) { return static_cast<double>(p); })
        .def("__neg__",   [](Player p) { return -p; })
        .def_static("calc_score", static_cast<double (*)(Player, Player)>(&CalcScore))
        .def_static("calc_score", static_cast<double (*)(Player, double)>(&CalcScore));

    // Definition of Position struct
    py::class_<Position>(mod, "Position", "Gomoku board positions")
        .def(py::init<int>()) // id
        .def(py::init<int, int>()) // (x, y)
        .def_readwrite("id", &Position::id)
        .def_property("x", &Position::x, [](Position& p, int x) { p.id = p.y() * WIDTH + x; })
        .def_property("y", &Position::y, [](Position& p, int y) { p.id += (y - p.y()) * WIDTH; })
        .def("__int__",  [](const Position& p) { return p.id; })
        .def("__hash__", [](const Position& p) { return std::hash<Position>()(p); })
        .def("__len__",  [](const Position& p) { return 2; })
        .def("__repr__", [](const Position& p) { return py::str("Position{}").format(std::to_string(p)); })
        .def("__str__",  [](const Position& p) { return std::to_string(p); })
        .def("__iter__", [](const Position& p) {
            auto dict = py::dict("x"_a = p.x(), "y"_a = p.y()); // READ-ONLY ITERATOR
            return py::make_iterator(dict.begin(), dict.end(), py::return_value_policy::copy); // ATTENTION: potential early-stop bug when work with iter()
        });
        


    // Definition of Board class
    py::class_<Board>(mod, "Board", "Gomoku game board")
        .def(py::init<>())
        .def("apply_move",  &Board::applyMove, py::arg("move"), py::arg("checkVictory") = true)
        .def("revert_move", &Board::revertMove, py::arg("count") = 1)
        .def("random_move", &Board::getRandomMove)
        .def("check_move",  &Board::checkMove)
        .def("check_end",   &Board::checkGameEnd)
        .def("reset",       &Board::reset)
        .def_readonly("move_record", &Board::m_moveRecord)
        .def_property_readonly("last_move", [](const Board& b) {
            return b.m_moveRecord.empty() ? Position(-1) : b.m_moveRecord.back();
        })
        .def_property_readonly("move_counts", [](const Board& b) {
            py::dict move_counts;
            for (auto player : { Player::Black, Player::None, Player::White }) {
                move_counts[py::cast(player)] = b.moveCounts(player);
            }
            return move_counts;
        })
        .def_property_readonly("move_states", [](const Board& b) {
            py::dict move_states;
            for (auto player : { Player::Black, Player::None, Player::White }) {
                // reshape to a square board
                move_states[py::cast(player)] = py::array(py::dtype("uint8"), { HEIGHT, WIDTH }, b.moveStates(player).data()); 
            }
            return move_states;
        })
        .def_property_readonly("status", [](const Board& b) {
            auto status = b.status();
            return py::dict(
                "is_end"_a = status.end,
                "cur_player"_a = status.curPlayer,
                "winner"_a = status.winner
            );
        })
        .def("encoded_states", [](const Board& b) {
            py::array_t<unsigned char> states({ 6, (int)HEIGHT, (int)WIDTH }); // 先y再x

            int index = 0;  // index将在流式初始化过程中自增
            for (auto player : { b.m_curPlayer, -b.m_curPlayer, Player::None }) {
                std::copy(b.moveStates(player).begin(), b.moveStates(player).end(), states.mutable_data(index++));
            }
            for (int i = 0; i <= 1; ++index, ++i) {
                std::fill(states.mutable_data(index), states.mutable_data(index) + HEIGHT * WIDTH, 0);
                if (b.m_moveRecord.size() > i) {
                    auto position = *(b.m_moveRecord.rbegin() + i);
                    *states.mutable_data(index, position.y(), position.x()) = 1;
                }
            }
            std::fill(states.mutable_data(index), states.mutable_data(index) + HEIGHT * WIDTH, b.m_curPlayer == Player::Black);

            return states;
        }, "Feature planes: [X_t, Y_t, Z_t, y_t-1, x_t-2, C<is_black>]")
        .def("__repr__", [](const Board& b) { return py::str("Board(cur_player: {})").format(std::to_string(b.m_curPlayer)); })
        .def("__str__",  [](const Board& b) { return std::to_string(b); });
}
