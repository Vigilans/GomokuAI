if __name__ == "__main__":
    from __init__ import PolicyValueNetwork, DataHelper
else:
    from .model_tf import PolicyValueNetwork
    # from .model_keras import PolicyValueNetwork
    from .data_helper import DataHelper

from config import TRAINING_CONFIG as config
import itertools


class TrainingPipeline:
    def __init__(self, model_file=None):
        """
        Training hyper parameters
        """
        self.lr = config["learning_rate"]
        self.lr_multiplier = 1.0
        self.momentum = config["momentum"]
        self.kl_target = config["kl_target"]
        self.batch_size = config["batch_size"]
        self.num_epoches = config["num_epoches"]

        """
        Record variables during training
        """
        self.total_steps = 0
        self.best_win_rate = 0.0
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

    def repetitive_generator(generator, num_epoches):
        """
        Since a mini-batch is randomly sampled from buffer,
        we will not iterate through a whole dataset.
        Therefore, multiple epoches can be defined as
        iterating through one mini-batch multiple times.
        """
        while True:
            mini_batch = next(generator)
            for i in range(num_epoches):
                yield mini_batch

    def train_network(self):
        """
        Start training, select data from buffer and feed into network.
        Using Mini-batch SGD training strategy.
        """
        loss, entropy, kl = self.network.train_step({
            "lr": self.lr * self.lr_multiplier,
            "momentum": self.momentum,
            "kl_target": self.kl_target
        }, self.num_epoches)

        # tune the learning rate according to KL divergence
        if kl > self.kl_target * 2 and self.lr_multiplier > 0.1:
            self.lr_multiplier /= 1.5
        elif kl < self.kl_target / 2 and self.lr_multiplier < 10:
            self.lr_multiplier *= 1.5

        print("step {} - loss: {}, entropy: {}, kl: {}, lr: {}".format(
            self.total_steps, loss, entropy, kl, self.lr * self.lr_multiplier
        ))

    async def evaluate_network():
        pass

    def run(self):
        # Initialize data generation
        generator = self.data_helper.generate_batch(self.batch_size)
        generator = self.repetitive_generator(generator, self.num_epoches)
        self.network.build_dataset(generator, self.num_epoches)

        # Start training
        for self.total_steps in itertools.count(1):
            # train network with multiple epoches (may be early stopped)
            self.train_network()
            # asynchronously evaluate network performance
            if self.total_steps % self.eval_period == 0:
                self.evaluate_network()


if __name__ == "__main__":
    try:
        pipeline = TrainingPipeline(config["model_file"])
        pipeline.run()
    except KeyboardInterrupt:
        pipeline.network.save_model("model-latest")
        print("\nQuit")
