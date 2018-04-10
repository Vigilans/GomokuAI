from core import Player, Board
import numpy as np


def dual_play(agents, board=None, verbose=False):
    """
    Play with 2 players.
    Params:
      players: { Player.black: agent1, Player.white: agent2 }.
      board:   initial board state. Start player will be determined here.
      verbose: if true, then return value will be in the form of training data.
    Returns:
      if verbose set to True:
        [(state_inputs, final_score, action_probs)]
      else:
        winner
    """
    if board is None:
        board = Board()

    if verbose is True:
        result = []
    else:
        result = Player.none

    while(True):
        # set the current agent
        cur_agent = agents[board.status["cur_player"]]

        # evaluate board state and get action
        if verbose is True:
            _, action_probs, next_move = cur_agent.eval_state(board)
            result.append((
                board.encoded_states(),
                board.status["cur_player"],
                action_probs
            ))
        else:
            next_move = cur_agent.get_action(board)
        # update board
        board.apply_move(next_move)

        # end judge
        if board.status["is_end"]:
            winner = board.status["winner"]
            # format output result
            if verbose is True:
                for state in result:
                    state[1] = Player.get_score(state[1], winner)
            else:
                result = winner

            return result


def eval_agent(agents, num_games=5, board=None):
    """
    Eval the performance of two agents by multiple game simulation.
    Params:
      agents: [agent1, agent2]
      num_games: number of games simulated, default to BO5.
      board: a pre-init board can be passed to avoid re-construct.
    Returns:
      [win_rate(a) for a in agents]
    """
    print("---------Evaluating agents-------------")

    players = [Player.black, Player.white]
    win_cnts = np.zeros(2)

    for i in range(num_games):
        winner = dual_play(dict(zip(players, agents)), board)
        try:
            winner_idx = players.index(winner)
            win_cnts[winner_idx] += 1
        except ValueError:  # tie
            win_cnts += 0.5
        players.reverse()  # exchange the start player

    win_rates = win_cnts / num_games

    print("Win rate:")
    print("{}: {}".format(agents[0], win_rates[0]))
    print("{}: {}".format(agents[1], win_rates[1]))
    print("---------------------------------------")
    return win_rates.tolist()


def botzone_interact(agent):
    """
    Play interactively according to the botzone interface.
    Returns:
        None
    """
    print("TODO!!!")
