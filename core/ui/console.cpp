#include <string>
#include <iostream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include "lib/include/Game.h"
#include "lib/include/MCTS.h"


using json = nlohmann::json;

using namespace std;

//namespace Gomoku {
//void to_json(json& j, const Position& p) {
//    j = { { "x", p.x() }, { "y", p.y() } };
//}
//
//void from_json(const json& j, Position& p) {
//    p.id = static_cast<int>(j["y"]) * WIDTH + static_cast<int>(j["x"]);
//}
//}
//
//int main(int argc, const char* argv[]) {
//    json input, output;
//    cin >> input;
//
//    Gomoku::Position firstMove = input["requests"][0];
//    Gomoku::Player player = firstMove.x() == -1 && firstMove.y() == -1 ? Gomoku::Player::Black : Gomoku::Player::White;
//   
//    Gomoku::Board board;
//    Gomoku::MCTS mcts(player);
//
//    int turnID = input["responses"].size();
//
//    for (int i = 0; i < turnID; i++) {
//        if (i != 0) {
//            board.applyMove(input["requests"][i]);
//        }
//        board.applyMove(input["responses"][i]);
//    }
//    board.applyMove(input["requests"][turnID]);
//
//
//    Gomoku::Position nextMove = mcts.getNextMove(board);
//    output["response"] = { {"x", nextMove.x() }, { "y", nextMove.y() } };
//    cout << output << endl;
//
//    return 0;
//}

using namespace Gomoku;

int main() {
    std::ios::sync_with_stdio(false);

    Board board;
    MCTS mcts;

    cout << std::to_string(board);
    for (int x, y; board.m_curPlayer != Player::None;) {
        cout << "\nYour  (x, y): ";
        cin >> hex >> x >> y;
        Position player_move(x - 1, y - 1);
        switch (board.applyMove(player_move)) {
        case Player::Black:
        case Player::None:
            continue;
        }
        mcts.stepForward(player_move);

        //auto cpu_move = board.getRandomMove();
        //auto cpu_move = MCTS(player_move, Player::Black).getNextMove(board);
        auto cpu_move = mcts.getNextMove(board);
        cout << "CPU's (x, y): " << cpu_move.x() + 1 << " " << cpu_move.y() + 1 << endl;
        board.applyMove(cpu_move);
        //cout << string(WIDTH - 3, ' ') << "Board\n";
        cout << endl << std::to_string(board);
    }

    cout << "\nWinner: " << (board.m_winner == Player::Black ? "You" : "CPU") << "!\n\n";

    return 0;
}