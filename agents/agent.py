if __name__ == "__main__":
    import __init__ as __pkg_init__

from core import Position, Board, GameConfig as Game
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
        # if not overrided, simply return uniform evaluation and random move
        return (
            0.0,
            np.full((Game["width"], Game["height"]), 1 / Game["board_size"]),
            self.get_action(state)
        )

    def reset(self):
        pass

    def __str__(self):
        return "Base Random Agent"


if __name__ == "__main__":
    from utils import botzone_interact
    botzone_interact(Agent())
