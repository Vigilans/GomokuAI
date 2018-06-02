from core import Player, GameConfig as Game
from network.data_helper import augment_game_data
from agents import RandomMCTSAgent
from agents.utils import dual_play
import numpy as np


def test_augment_game_data():
    agent = RandomMCTSAgent(c_iterations=400)
    game_data = dual_play({
        Player.black: agent,
        Player.white: agent
    }, verbose=True)
    augmented_data = augment_game_data(game_data)

    fake_data = []
    for data in game_data:
        fake_data.append((*data[:2], np.arange(Game["board_size"])))
    id_data = augment_game_data(fake_data)

    test_data = []
    for i in range(len(augmented_data)):
        test_data.append([
            id_data[i][-1].flatten(),
            *[s.flatten() for s in augmented_data[i][0]],
            augmented_data[i][2].flatten()
        ])

    for i, ref in enumerate(test_data[::8]):
        for j in range(1, 8):
            print(test_data[8*i+j][-1].reshape(15, 15))
            test = test_data[8*i+j]
            ids = test[0]
            for a, b in zip(test[1:], ref[1:]):
                assert np.all(a == b[ids])
