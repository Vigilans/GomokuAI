import json
import os
import numpy as np
from subprocess import PIPE, Popen
from config import DATA_CONFIG
from core import GameConfig as Game
from core import Position
from .agent import Agent


class BotzoneAgent(Agent):
    def __init__(self, program, keep_alive=False,
                 working_dir=DATA_CONFIG["data_path"]):
        self.program = program
        self.working_dir = os.path.realpath(working_dir)
        self.keep_alive = keep_alive  # TODO: implement keep_alive version

    def get_action(self, state):
        return self._communicate(state)

    def eval_state(self, state):
        action = self._communicate(state)
        action_probs = np.zeros(Game["board_size"])
        action_probs[int(action)] = 1.0
        return 0.0, action_probs, action

    def _parse_state(self, state):
        # cur_player: 0 for black, 1 for white
        offset = len(state.move_record) % 2
        padding = [Position(-1, -1)] * (1 - offset)
        return json.dumps({
            "requests": padding + state.move_record[1 - offset::2],
            "responses": state.move_record[offset::2]
        }, default=lambda obj: dict(obj))

    def _communicate(self, state):
        bot = Popen(self.program, shell=True, stdin=PIPE, cwd=self.working_dir,
                    stdout=PIPE, universal_newlines=True)
        output, _ = bot.communicate(self._parse_state(state))
        bot.terminate()
        return Position(*json.loads(output)["response"].values())

    def __repr__(self):
        return f"Botzone Agent at <{self.working_dir}/{self.program}>"


def main():
    from .utils import botzone_interface
    program_path = "your/path/to/program"
    botzone_interface(BotzoneAgent(program_path))


if __name__ == "__main__":
    main()
