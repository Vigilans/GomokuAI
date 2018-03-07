#include <string>
#include <iostream>
#include "Game.hpp"
#include "MCTS.hpp"
#include "jsoncpp/json.h"

using namespace std;

Gomoku::Position jsonToPosition(const Json::Value& json) {
    return Gomoku::Position(json["x"].asInt(), json["y"].asInt());
}

Json::Value positionToJSON(const Gomoku::Position position) {
    Json::Value move;
    move["x"] = position.x();
    move["y"] = position.y();
    return move;
}

int main(int argc, const char* argv[]) {
    Gomoku::MCTS mcts(Gomoku::Player::Black);
    Gomoku::Board board;

    string str;
    getline(cin, str);
    Json::Reader reader;
    Json::Value input;
    reader.parse(str, input);

    int turnID = input["responses"].size();
    for (int i = 0; i < turnID; i++) {
        board.applyMove(jsonToPosition(input["requests"][i]));
        board.applyMove(jsonToPosition(input["responses"][i]));
    }
    board.applyMove(jsonToPosition(input["requests"][turnID]));
    
    Json::Value ret;
    ret["response"] = positionToJSON(mcts.getNextMove(board));
    Json::FastWriter writer;
    cout << writer.write(ret) << endl;

    return 0;
}