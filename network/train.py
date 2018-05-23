if __name__ in ["__main__", "__mp_main__"]:
    from __init__ import PolicyValueNetwork, DataHelper
else:
    from .model_tf import PolicyValueNetwork
    # from .model_keras import PolicyValueNetwork
    from .data_helper import DataHelper

from config import TRAINING_CONFIG as config, MCTS_CONFIG
from agents import MCTSAgent, PyConvNetAgent, TraditionalAgent
from agents.utils import eval_agents
import itertools


class TrainingPipeline:
    def __init__(self, model_name=None):
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
        self.eval_period = config["eval_period"]
        self.eval_rounds = config["eval_rounds"]
        self.total_steps = 0
        self.ref_iterations = 400
        self.best_win_rate = 0.0  # base win rate

        """
        Policy Network with certain interface
        """
        self.network = PolicyValueNetwork(model_name)
        self.network.compile()

        """
        Data Helper with chosen agents to generate data
        """
        self.data_helper = DataHelper()

        if model_name is not None:
            self.restore_model(model_name)

    def make_repetitive_generator(self, gen_iter, num_epoches):
        """
        Since a mini-batch is randomly sampled from buffer,
        we are not able to iterate through a whole dataset.
        Therefore, multiple epoches can be defined as
        iterating through one mini-batch multiple times.
        """
        def generator():
            while True:
                mini_batch = next(gen_iter)
                for i in range(num_epoches):
                    yield mini_batch
        return generator

    def train_network(self):
        """
        Start training, select data from buffer and feed into network.
        Using Mini-batch SGD training strategy.
        """
        loss, entropy, kl, epoches = self.network.train_step({
            "lr": self.lr * self.lr_multiplier,
            "momentum": self.momentum,
            "kl_target": self.kl_target
        }, self.num_epoches)

        # tune the learning rate according to KL divergence
        if kl > self.kl_target * 2 and self.lr_multiplier > 0.1:
            self.lr_multiplier /= 1.5
        elif kl < self.kl_target / 2 and self.lr_multiplier < 10:
            self.lr_multiplier *= 1.5

        print(
            "step %d -" % self.total_steps,
            "loss: %.5f," % loss,
            "entropy: %.5f," % entropy,
            "kl: %.5f," % kl,
            "lr: %.5f," % (self.lr * self.lr_multiplier),
            "epoches: %d" % epoches
        )

    def save_model(self, name, rich_info=False):
        if rich_info:
            name = "{}-{}-{}-{:.2f}".format(
                name, self.total_steps, self.ref_iterations, self.best_win_rate
            )
        self.network.save_model(name)

    def restore_model(self, name):
        self.network.restore_model(name)
        model_infos = self.network.model_file.split('/')[-1].split('-')
        self.total_steps = int(model_infos[1])
        self.ref_iterations = int(model_infos[2])
        self.best_win_rate = float(model_infos[3])

    def evaluate_network(self):
        base_agent = PyConvNetAgent(
            self.network, MCTS_CONFIG["c_puct"],
            c_iterations=MCTS_CONFIG["c_iterations"]
        )
        ref_agent = TraditionalAgent(
            MCTS_CONFIG["c_puct"],
            c_iterations=self.ref_iterations
        )

        win_rate, _ = eval_agents([base_agent, ref_agent], self.eval_rounds)
        if win_rate > self.best_win_rate:
            print("New best policy found!")
            self.save_model("best_model-{}".format(self.ref_iterations))
            if win_rate == 1.0:
                self.ref_iterations += 600
                self.best_win_rate = 0.0
            else:
                self.best_win_rate = win_rate

        # checkpoint
        self.save_model("current_model", rich_info=True)

    def run(self):
        # Initialize data generation
        gen_iter = self.data_helper.generate_batch(self.batch_size)
        generator = self.make_repetitive_generator(gen_iter, self.num_epoches)
        self.network.build_dataset(generator, self.num_epoches)

        # Start training
        for self.total_steps in itertools.count(self.total_steps + 1):
            # train network with multiple epoches (may be early stopped)
            self.train_network()
            # asynchronously evaluate network performance
            if self.total_steps % self.eval_period == 0:
                self.evaluate_network()  # who knows how to eval in new proc???


if __name__ == "__main__":
    try:
        pipeline = TrainingPipeline(config["model_file"])
        pipeline.run()
    except KeyboardInterrupt:
        pipeline.save_model("current_model", rich_info=True)
        print("\nQuit")
