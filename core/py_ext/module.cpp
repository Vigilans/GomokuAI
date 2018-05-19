#include "pch.h"
#include "game_ext.hpp"
#include "mcts_ext.hpp"
#include "policy_ext.hpp"

// General definitions
PYBIND11_MODULE(CorePyExt, mod) {
    mod.doc() = "Gomoku AI core module";
    
    Game_Ext(mod);
    MCTS_Ext(mod);
    Policy_Ext(mod);
}