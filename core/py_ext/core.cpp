#include "pch.h"
#include "game_ext.hpp"
#include "mcts_ext.hpp"

// General definitions
PYBIND11_MODULE(core, mod) {
    mod.doc() = "Gomoku AI core module";
    Game_Ext(mod);
    MCTS_Ext(mod);
}