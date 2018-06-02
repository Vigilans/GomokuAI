import numpy as np
from core import GameConfig as Game
from core import Position
from .agent import Agent


class ConsoleAgent(Agent):
    def __init__(self):
        pass

    def get_action(self, state):
        action = input("Input your move: ")
        for separator in [' ', ',']:
            if separator in action:
                return Position(map(int, action.split(separator)))

    def eval_state(self, state):
        action = input("Input your move: ")
        state_value = input("Input your state value estimation: ")
        for separator in [' ', ',']:
            if separator in action:
                action = Position(map(int, action.split(separator)))
                action_probs = np.zeros(Game["board_size"])
                action_probs[int(action)] = 1.0
                return state_value, action_probs, action

    def __repr__(self):
        return "Human Console Agent"


def main():
    from .utils import botzone_interface
    botzone_interface(ConsoleAgent())


if __name__ == "__main__":
    main()
