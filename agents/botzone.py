from .agent import Agent
from core import Position, GameConfig
from subprocess import Popen, PIPE
import numpy as np
import json
import os


class BotzoneAgent(Agent):
    def __init__(self, program_path, keep_alive=False):
        self.path, self.program = os.path.split(program_path)
        self.path = os.path.realpath(self.path)
        self.keep_alive = keep_alive  # TODO: implement keep_alive version

    def get_action(self, state):
        return self._communicate(state)

    def eval_state(self, state):
        action = self._communicate(state)
        action_probs = np.zeros(GameConfig["board_size"])
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
        bot = Popen(self.program, shell=True, stdin=PIPE, cwd=self.path,
                    stdout=PIPE, universal_newlines=True)
        output, error = bot.communicate(self._parse_state(state))
        bot.terminate()
        return Position(*json.loads(output)["response"].values())

    def __repr__(self):
        return "Botzone Agent at <{}>".format(self.path)


if __name__ == "__main__":
    from .utils import botzone_interface
    program_path = "your/path/to/program"
    botzone_interface(BotzoneAgent(program_path))
