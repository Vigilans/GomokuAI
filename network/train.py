if __name__ == "__main__":
    from __init__ import PolicyValueNetwork, DataHelper
else:
    from .model_tf import PolicyValueNetwork
    # from .model_keras import PolicyValueNetwork
    from .data_helper import DataHelper

from core import Player
from agents import *
from config import TRAINING_CONFIG as config


class TrainingPipeline:
    def __init__(self, model_file=None):
        """
        Training hyper parameters
        """
        self.learning_rate = config["learning_rate"]
        self.momentum = config["momentum"]
        self.batch_size = config["batch_size"]
        self.num_epoches = config["num_epoches"]

        """
        Record variables during training
        """
        self.total_steps = 0  # TODO: updated from model
        self.eval_period = config["eval_period"]

        """
        Policy Network with certain interface
        """
        self.network = PolicyValueNetwork(model_file)
        self.network.compile({
            "lr": self.learning_rate,
            "momentum": self.momentum
        })

        """
        Data Helper with chosen agents to generate data
        """
        self.data_helper = DataHelper()

    def start_data_pipeline():
        pass

    def train_network(self):
        """
        Start training, select data from buffer and feed into network.
        Using Mini-batch SGD training strategy.
        Network will be trained asynchrously and continuously.
        """
        mini_batch = random.sample(self.buffer, self.batch_size)

        # start training
        for i in range(self.num_epoches):
            """
            Since a mini-batch is randomly sampled from buffer,
            we will not iterate through a whole dataset.
            Therefore, multiple epoches can be defined as
            iterating through one mini-batch multiple times.
            """
            self.network.train_step(
                [sample[0] for sample in mini_batch],  # state_inputs
                [sample[1] for sample in mini_batch],  # state_value
                [sample[2] for sample in mini_batch],  # action_probs
                self.learning_rate,
                self.momentum
            )

    def run(self):
        generator = self.data_helper.generate_batch(self.batch_size)
        self.network.build_dataset(generator)
        while (True):
            # self.buffer.extend(self.data_helper.generate_game_data())
            # print(self.buffer.end)
            # print(len(self.buffer))
            # if len(self.buffer) > self.batch_size:
            #     self.train_network()
            
            self.network.model.fit_generator(
                self.data_helper.generate_batch(self.batch_size),
                self.batch_size
            )
            # self.total_steps += self.batch_size
            # if self.total_steps % self.eval_period == 0:
            #     winrate, _ = eval_agents([AlphaZeroAgent(self.network), DefaultMCTSAgent()])


if __name__ == "__main__":
    try:
        pipeline = TrainingPipeline("model-latest")
        pipeline.run()
    except KeyboardInterrupt:
        pipeline.network.save_model("model-latest")
        print("\nQuit")
