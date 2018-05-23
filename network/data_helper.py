if __name__ in ["__main__", "__mp_main__"]:
    import __init__ as __pkg_init__

from core import Board, Player, GameConfig
from agents import AGENT_MAP
from agents.utils import dual_play
from config import DATA_CONFIG as config, TRAINING_CONFIG
from multiprocessing import Process, Manager
import numpy as np
import random
import h5py
import os


def parse_agents(conf=config["agents"]):
    # pad the agent num to 2 (same object will be refed when self-play)
    return [AGENT_MAP[name](*args) for name, args in conf] * (3 - len(conf))


def augment_game_data(data):
    augmented_data = []
    for states, value, probs in data:
        for i in range(4):
            rot_states = np.array([
                np.rot90(
                    state.reshape(GameConfig["width"], GameConfig["height"]), i
                ) for state in states
            ])
            rot_probs = np.rot90(
                probs.reshape(GameConfig["width"], GameConfig["height"]), i
            )
            augmented_data.append((rot_states, value, rot_probs))

            flip_states = np.array([
                np.fliplr(state) for state in rot_states
            ])
            flip_probs = np.fliplr(
                rot_probs
            )
            augmented_data.append((flip_states, value, flip_probs))
    return augmented_data


def simulate_game_data(board, agents):
    players = [Player.black, Player.white]
    random.shuffle(players)
    data = dual_play(dict(zip(players, agents)), board, True)
    for elem in [board, *agents]:
        elem.reset()
    return augment_game_data(data)


def run_proc(buffer, maxlen, lock, keep_alive=True,
             board=None, agents=None):
    """ Multiprocessing target funcion """
    if board is None:
        board = Board()
    if agents is None:
        agents = parse_agents()
    while True:
        data = simulate_game_data(board, agents)
        print("Finished one episode with {} samples.".format(len(data)))
        with lock:
            while len(buffer) >= maxlen:
                buffer.pop(0)
            buffer.extend(data)
        if not keep_alive:
            return


class DataHelper:
    def __init__(self, data_files=config["data_files"]):
        # self._agents = agents
        self._data_files = data_files

        """
        Multicore data generating managers
        """
        self._manager = Manager()
        self.lock = self._manager.Lock()

        """
        Multicore processes config
        """
        self._process_num = config["process_num"]
        self._processes = []

        """
        Multicore data buffer
        Buffer element structure:
        (state_inputs, state_value, action_probs)
        """
        self.buffer_size = config["buffer_size"]
        self.buffer = self._manager.list()

    def generate_batch(self, batch_size):
        """
        Batch data generator
        """
        # consume data files
        for file_name in self._data_files:
            data_file = "{}/{}".format(config["data_path"], file_name)
            if not os.path.exists(data_file):
                continue
            with h5py.File(data_file, 'r') as hf:
                for mini_batch in zip(
                    hf["state_batch"],
                    hf["value_batch"],
                    hf["policy_batch"]
                ):
                    yield mini_batch
            # mark the data file as consumed
            os.rename(file_name, file_name + ".consumed")

        # then generate data by automatic game play
        self.init_simulation()
        while True:
            if (len(self.buffer) > batch_size):
                mini_batch = random.sample(list(self.buffer), batch_size)
                yield self.parse_batch(mini_batch)

    def parse_batch(self, mini_batch):
        state_batch, value_batch, probs_batch = [
            np.array([sample[i] for sample in mini_batch]) for i in range(3)
        ]
        return state_batch, value_batch, probs_batch

    def init_simulation(self):
        for i in range(self._process_num):
            process = Process(target=run_proc,
                              args=(self.buffer, self.buffer_size, self.lock))
            process.daemon = True
            process.start()
            self._processes.append(process)


if __name__ == "__main__":
    with h5py.File("{}/latest.train.hdf5".format(config["data_path"]), 'a') as hf:
        try:
            helper = DataHelper(data_files=[])
            generator = helper.generate_batch(TRAINING_CONFIG["batch_size"])
            for state_batch, value_batch, probs_batch in generator:
                board = Board()
                agents = parse_agents()
                for batch_name in ("state_batch", "value_batch", "probs_batch"):
                    if batch_name not in hf:
                        shape = locals()[batch_name].shape
                        # print(shape)
                        hf.create_dataset(batch_name, (0, *shape),
                                          maxshape=(None, *shape))
                    hf[batch_name].resize(hf[batch_name].shape[0] + 1, axis=0)
                    hf[batch_name][-1] = locals()[batch_name]
                # prevent main proc from generating data too quick
                run_proc(helper.buffer, helper.buffer_size, helper.lock,
                         False, board, agents)
        except KeyboardInterrupt:
            print("\nQuit")
