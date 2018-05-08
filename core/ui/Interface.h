#ifndef GOMOKU_INTERFACE_H_
#define GOMOKU_INTERFACE_H_
#include <iostream>
#include <iomanip>
#include <string>
#include <nlohmann/json.hpp>
#include "Agent.h"

namespace Gomoku {

using json = nlohmann::json;
void to_json(json& j, const Position& p) { j = { { "x", p.x() },{ "y", p.y() } }; }
void from_json(const json& j, Position& p) { p = Position{ j["x"], j["y"] }; }

namespace Interface {

inline int BotzoneInterface(Agent& agent) {
    using namespace std;

    Board board;
    json input, output;

    cin >> input;

    int turnID = input["responses"].size();
    for (int i = 0; i < turnID; i++) {
        board.applyMove(input["requests"][i], false);  // requests size: n + 1
        board.applyMove(input["responses"][i], false); // responses size: n
    }
    board.applyMove(input["requests"][turnID], false); // play newest move

    agent.syncWithBoard(board);
    output["response"] = agent.getAction(board); // response to newest move

    cout << output << endl;

    return 0;
}

inline int KeepAliveBotzoneInterface(Agent& agent) {
    using namespace std;

    Board board;

    while (true) {
        json input, output;

        cin >> input;

        board.applyMove(input["requests"], false); // play newest move
        agent.syncWithBoard(board);
        output["response"] = agent.getAction(board); // response to newest move

        cout << output << "\n";
        cout << ">>>BOTZONE_REQUEST_KEEP_RUNNING<<<" << endl; // stdio flushed by endl
    }

    return 0;
}

inline int ConsoleInterface(Agent& agent0, Agent& agent1) {
    using namespace std;

    Board board;

    srand(time(nullptr));
    int black_player = rand() % 2;
    Agent* agents[2] = { &agent0, &agent1 };
    int index[2] = { !black_player, black_player };
    auto get_agent = [&](Player player) {
        auto i = index[((int)player + 1) / 2];
        return tie(*agents[i], i);
    };

    
    cout << std::to_string(Player::Black) << ": " << index[1] << "." << agents[1]->name().data() << endl;
    cout << std::to_string(Player::White) << ": " << index[0] << "." << agents[0]->name().data() << endl;
    cout << endl << std::to_string(board);
    for (auto cur_player = Player::Black; ;) {
        auto[agent, index] = get_agent(cur_player);
        agent.syncWithBoard(board);
        auto move = agent.getAction(board);
        if (auto result = board.applyMove(move); result != cur_player) {
            printf("\n%d.%s's move: (%x, %x):\n\n", index, agent.name().data(), move.x() + 1, move.y() + 1);
            cout << std::to_string(board);
            if (result == Player::None) {
                auto [winner, idx] = get_agent(board.m_winner);
                printf("\nGame end. Winner: %d.%s", idx, winner.name().data());
                break;
            } else if (result == -cur_player) {
                cur_player = -cur_player;
                continue;
            }
        } else {
            cout << "Invalid move." << endl;
            continue;
        }
    }

    return 0;
}

}
}


#endif // !GOMOKU_INTERFACE_H_
