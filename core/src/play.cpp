#include <string>
#include <iostream>
#include "Game.hpp"
#include "MCTS.hpp"
#include <jsoncpp/json.h>

using namespace std;

int main(int argc, const char* argv[]) {
    string str;
    getline(cin, str);
    Json::Reader reader;
    Json::Value input;
    reader.parse(str, input);

    auto firstMove = jsonToPosition(input["requests"][int(0)]);
    auto player = firstMove.x() == -1 && firstMove.y() == -1 ? Gomoku::Player::Black : Gomoku::Player::White;

    Gomoku::Board board;
    Gomoku::MCTS mcts(player);


    int turnID = input["responses"].size();
    for (int i = 0; i < turnID; i++) {
        if (i != 0) {
            board.applyMove(jsonToPosition(input["requests"][i]));
        }
        board.applyMove(jsonToPosition(input["responses"][i]));
    }
    board.applyMove(jsonToPosition(input["requests"][turnID]));

    Json::Value ret;
    ret["response"] = positionToJSON(mcts.getNextMove(board));
    Json::FastWriter writer;
    cout << writer.write(ret) << endl;

    return 0;
}