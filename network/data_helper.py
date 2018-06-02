import os
import random
from multiprocessing import Manager, Process, Event

import h5py
import numpy as np

from agents import AGENT_MAP
from agents.utils import dual_play
from config import DATA_CONFIG, TRAINING_CONFIG
from core import GameConfig as Game
from core import Board, Player


def parse_schedule(level=-1):
    supervisor, candidates = DATA_CONFIG["schedule"].values()
    if candidates[level] is not None:
        return [supervisor, candidates[level]]
    else:
        # return [("alphazero", {"model_path": "latest"})]  # self-play
        raise NotImplementedError("No trivial metadata for alphazero currently")


def parse_agents_meta(metadata):
    # pad the agent num to 2 (same object will be refered when self-play)
    return [AGENT_MAP[name](**kwargs) for name, kwargs in metadata] * (3 - len(metadata))


def parse_batch(mini_batch):
    state_batch, value_batch, probs_batch = [
        np.array([sample[i] for sample in mini_batch]) for i in range(3)
    ]
    return state_batch, value_batch, probs_batch


def augment_game_data(data):
    augmented_data = []
    for states, value, probs in data:
        for i in range(4):
            rot_states = np.array([
                np.rot90(state, i) for state in states
            ])
            rot_probs = np.rot90(
                probs.reshape(Game["height"], Game["width"]), i
            )
            augmented_data.append((rot_states, value, rot_probs.flatten()))

            flip_states = np.array([
                np.fliplr(state) for state in rot_states
            ])
            flip_probs = np.fliplr(
                rot_probs
            )
            augmented_data.append((flip_states, value, flip_probs.flatten()))
    return augmented_data


def simulate_game_data(board, agents):
    players = [Player.black, Player.white]
    random.shuffle(players)
    data = dual_play(dict(zip(players, agents)), board, True)
    for elem in [board, *agents]:
        elem.reset()
    return augment_game_data(data)


def run_proc(buffer, maxlen, lock, sigexit,
             agents_meta, board=None):
    """ 
    Multiprocessing target funcion 
    """
    if board is None:
        board = Board()
    agents = parse_agents_meta(agents_meta)
    while True:
        data = simulate_game_data(board, agents)
        print(f"Finished one episode with {len(data)} samples.")
        with lock:
            if len(buffer) >= maxlen:
                buffer = buffer[len(buffer) - maxlen:]
            buffer.extend(data)
        if sigexit.is_set():
            return


class DataHelper:
    def __init__(self, data_files=DATA_CONFIG["data_files"]):
        """
        Metadata of data generators
        """
        self._agents_meta = parse_schedule()  # default to be self-play AlphaZeroAgent
        self._data_files = data_files

        """
        Multicore data generating managers
        """
        self._manager = Manager()
        self._exit = Event()
        self.lock = self._manager.Lock()

        """
        Multicore processes config
        """
        self._process_num = DATA_CONFIG["process_num"]
        self._processes = []

        """
        Multicore data buffer
        Buffer element structure:
        (state_inputs, state_value, action_probs)
        """
        self.buffer_size = DATA_CONFIG["buffer_size"]
        self.buffer = self._manager.list()

    def generate_batch(self, batch_size):
        """
        Batch data generator
        """
        # consume data files
        for file_name in self._data_files:
            data_file = f"{DATA_CONFIG['data_path']}/{file_name}"
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
                yield parse_batch(mini_batch)

    def set_agents_meta(self, level=None, agents_meta=None):
        """
        Overloaded method to set agents metadata.
        """
        if agents_meta is not None:
            self._agents_meta = agents_meta
        elif level is not None:
            self._agents_meta = parse_schedule(level)
        else:
            raise TypeError("One of 'level' and 'agents_meta' must be not None")

    def init_simulation(self):
        print(f"current agents: {[meta[0] for meta in self._agents_meta]}")
        for _ in range(self._process_num):
            process = Process(
                daemon=True, target=run_proc,
                args=(self.buffer, self.buffer_size, self.lock, self._exit, self._agents_meta)
            )
            process.start()
            self._processes.append(process)
        print("All data generators are successfully initialized.")

    def stop_simulation(self):
        self._exit.set()
        while not all([p.is_alive() for p in self._processes]):
            pass
        print("All data generators have stopped.")
        self._processs = []
        self._exit.clear()


def main():
    # Use alphazero self-play for data generation
    agents_meta = parse_schedule() 

    # worker variable of main process
    board = Board()
    sigexit = Event()
    sigexit.set()  # pre-set signal so main proc generator will iterate only once

    # subprocess data generator
    helper = DataHelper(data_files=[])
    helper.set_agents_meta(agents_meta=agents_meta)     
    generator = helper.generate_batch(TRAINING_CONFIG["batch_size"])

    # start generating
    with h5py.File(f"{DATA_CONFIG['data_path']}/latest.train.hdf5", 'a') as hf:    
        for state_batch, value_batch, probs_batch in generator:
            for batch_name in ("state_batch", "value_batch", "probs_batch"):
                if batch_name not in hf:
                    shape = locals()[batch_name].shape
                    hf.create_dataset(batch_name, (0, *shape), maxshape=(None, *shape))
                hf[batch_name].resize(hf[batch_name].shape[0] + 1, axis=0)
                hf[batch_name][-1] = locals()[batch_name]

            # prevent main proc from generating data too quick
            # since sigexit has been set, proc will iterate only once
            run_proc(helper.buffer, helper.buffer_size, helper.lock,
                     sigexit, agents_meta, board) 
            board.reset()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nQuit")
