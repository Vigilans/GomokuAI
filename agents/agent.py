from core import Player, Position, Board, GameConfig as Game
import numpy as np


class Agent:
    """ Agent base class, also serving as a RandomAgent. """
    def __init__(self):
        pass

    def get_action(self, state: Board) -> Position:
        """ Simply get the next position according to current state. """
        # if not overrided, simply return random move
        return state.random_move()

    def eval_state(self, state: Board) -> (float, np.array, Position):
        """
        Verbosely return the evaluation of board state.
        Returns:
          (state_value, action_probs, next_move)
        """
        # if not overrided, simply return random and uniform evaluation
        return (
            0.0,
            np.full((Game["width"], Game["height"]), 1 / Game["board_size"]),
            self.get_action(state)
        )

    def interact_play(self):
        """
        Play interactively according to the botzone interface.
        Returns:
            None
        """
        pass


def dual_play(players, board=None, verbose=False):
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
        cur_agent = players[board.status["cur_player"]]

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
