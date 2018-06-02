import numpy as np
from core import Board, Player


def dual_play(agents, board=None, verbose=False, graphic=False):
    """
    Play with 2 players.
    Params:
      agents:  { Player.black: agent1, Player.white: agent2 }.
      board:   initial board state. Start player will be determined here.
      verbose: if true, then return value will be in the form of training data.
    Returns:
      if verbose set to True:
        [(state_inputs, final_score, action_probs)]
        Each element is a numpy.array.
      else:
        winner
    """
    if board is None:
        board = Board()
    elif board.status["is_end"]:
        board.reset()

    if verbose is True:
        result = []
    else:
        result = Player.none

    while True:
        # set the current agent
        cur_agent = agents[board.status["cur_player"]]

        # evaluate board state and get action
        if verbose is True:
            _, action_probs, next_move = cur_agent.eval_state(board)
            result.append([
                board.encoded_states(),
                board.status["cur_player"], 
                action_probs
            ])
        else:
            next_move = cur_agent.get_action(board)
        # update board
        board.apply_move(next_move)
        if graphic:
            print(board)

        # end judge
        if board.status["is_end"]:
            winner = board.status["winner"]
            if graphic:
                print("Game ends. winner is {}.".format(winner))
            # format output result
            if verbose is True:
                result = [(
                    state[0],
                    np.array(Player.calc_score(state[1], winner)), 
                    state[2]
                ) for state in result]
            else:
                result = winner

            return result


def eval_agents(agents, num_games=9):
    """
    Eval the performance of two agents by multiple game simulation.
    Params:
      agents: [agent1, agent2]
      num_games: number of games simulated, default to BO9.
      board: a pre-init board can be passed to avoid re-construct.
    Returns:
      [win_rate(a) for a in agents]
    """
    print("---------Evaluating agents-------------")

    board = Board()
    players = [Player.black, Player.white]
    win_cnts = np.zeros(2)

    for i in range(num_games):
        winner = dual_play(dict(zip(players, agents)), board)
        try:
            win_idx = players.index(winner)
            win_cnts[win_idx] += 1
            print("Round {} ends, winner is <{}: {}>;".format(i + 1, winner, agents[win_idx]))
        except ValueError:  # tie
            win_cnts += 0.5
            print("Round {} ends, tie game;".format(i + 1))
        players.reverse()  # exchange the start player
        board.reset()
        [agent.reset() for agent in agents]

    win_rates = win_cnts / num_games

    print("Win rate:")
    print("{}: {}".format(agents[0], win_rates[0]))
    print("{}: {}".format(agents[1], win_rates[1]))
    print("---------------------------------------")
    return tuple(win_rates)


def botzone_interface(agent):
    """
    Play interactively according to the botzone interface.
    Returns:
        None
    """
    print("TODO!!!")


def keepalive_botzone_interface(agent):
    """
    Play interactively according to the botzone interface.
    Returns:
        None
    """
    print("TODO!!!")
