#ifndef GOMOKU_INTERFACE_H_
#define GOMOKU_INTERFACE_H_
#include <iostream>
#include <iomanip>
#include "Agent.h"

namespace Gomoku::Interface {

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
    output["debug"] = agent.debugMessage();

    cout << output << endl;

    return 0;
}

inline int KeepAliveBotzoneInterface(Agent& agent) {
    using namespace std;

    Board board;

    for (int turn = 0; ; ++turn) {
        json input, output;

        cin >> input;
        if (turn == 0) {
            input = input["requests"][0];
        }

        board.applyMove(input, false); // play newest move
        agent.syncWithBoard(board);
        board.applyMove(agent.getAction(board)); // response to newest move
        output["response"] = board.m_moveRecord.back(); 
        output["debug"] = agent.debugMessage();

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
    auto get_agent = [&](Player player) -> tuple<Agent&, int> {
        auto i = index[((int)player + 1) / 2];
        return { *agents[i], i };
    };

    cout << std::to_string(Player::Black) << ": " << index[1] << "." << agents[index[1]]->name().data() << endl;
    cout << std::to_string(Player::White) << ": " << index[0] << "." << agents[index[0]]->name().data() << endl;
    cout << endl << std::to_string(board);
    for (auto cur_player = Player::Black; ;) {
        auto[agent, index] = get_agent(cur_player);
        agent.syncWithBoard(board);
        auto move = agent.getAction(board);
        cout << "\nDebug Messages:" << agent.debugMessage() << "\n-------------------------\n";
        if (auto result = board.applyMove(move); result != cur_player) {
            printf("\n%d.%s's move: (%x, %x):\n\n", index, agent.name().data(), move.x() + 1, move.y() + 1);
            cout << std::to_string(board);
            if (result == Player::None) {
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

    auto [winner, idx] = get_agent(board.m_winner);
    printf("\nGame end. Winner: %d.%s\n", idx, winner.name().data());

    return idx;
}

}


#endif // !GOMOKU_INTERFACE_H_
