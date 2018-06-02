import itertools

from agents import AlphaZeroAgent, MCTSAgent
from agents.utils import eval_agents
from config import TRAINING_CONFIG, DATA_CONFIG, MCTS_CONFIG

# from .model_keras import PolicyValueNetwork
from .data_helper import DataHelper, parse_agents_meta
from .model_tf import PolicyValueNetwork


class TrainingPipeline:
    def __init__(self, model_name=None):
        """
        Training hyper parameters
        """
        self.num_epoches = TRAINING_CONFIG["num_epoches"]
        self.batch_size = TRAINING_CONFIG["batch_size"]
        self.kl_target = TRAINING_CONFIG["kl_target"]
        self.momentum = TRAINING_CONFIG["momentum"]
        self.lr = TRAINING_CONFIG["learning_rate"]
        self.lr_multiplier = 1.0
        
        """
        Record variables during training
        """
        self.eval_period = TRAINING_CONFIG["eval_period"]
        self.eval_rounds = TRAINING_CONFIG["eval_rounds"]
        self.total_steps = 0
        self.schedule_level = 0
        self.ref_iterations = MCTS_CONFIG["c_iterations"]
        self.best_win_rate = 0.0

        """
        Policy Network with generic interface
        """
        self.network = PolicyValueNetwork(model_name)
        self.network.compile()

        """
        Data Helper to generate data according to schedule
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
                for _ in range(num_epoches):
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

    def evaluate_network(self):
        # prepare agents metadata
        # agents_meta = [("alphazero", {"model_path": "latetst"})]
        agents_meta = [("alphazero", {"network": self.network, **MCTS_CONFIG})]
        supervisor, candidates = DATA_CONFIG["schedule"].values()
        if candidates[self.schedule_level] is not None:
            agents_meta.append(candidates[self.schedule_level])
        else:
            agents_meta.append(supervisor)
        
        if "mcts" in agents_meta[-1][0]:  # power up mcts candidates
            agents_meta[-1][1]["c_iterations"] = self.ref_iterations

        # start evaluation
        base_agent, ref_agent = parse_agents_meta(agents_meta)
        win_rate, _ = eval_agents([base_agent, ref_agent], self.eval_rounds)

        # best policy check
        if win_rate > self.best_win_rate:
            print("New best policy found!")
            self.save_model("best_model-{}-{}".format(
                agents_meta[-1][0], self.ref_iterations
            ))
            # schedule levelup check
            if win_rate >= 1.0 - 0.05*self.schedule_level:  # levelup threshold decay
                if isinstance(ref_agent, MCTSAgent):
                    self.ref_iterations += 2 * MCTS_CONFIG["c_iterations"]
                if not isinstance(ref_agent, MCTSAgent) or self.ref_iterations > 20000:
                    self.ref_iterations = MCTS_CONFIG["c_iterations"]
                    self.schedule_level += 1  # go to next candidate
                    self.data_helper.stop_simulation()
                    self.data_helper.set_agents_meta(level=self.schedule_level)
                    self.data_helper.init_simulation()
                self.best_win_rate = 0.0
            else:
                self.best_win_rate = win_rate

        # new checkpoint after every evaluation
        self.save_model("current_model", rich_info=True)

    def save_model(self, name, rich_info=False):
        if rich_info:
            name = "{}-{}-{}-{}-{:.2f}".format(
                name, self.total_steps, 
                self.schedule_level, self.ref_iterations, 
                self.best_win_rate
            )
        self.network.save_model(name)

    def restore_model(self, name):
        self.network.restore_model(name)
        if self.network.model_file:
            model_infos = self.network.model_file.split('/')[-1].split('-')
            self.total_steps = int(model_infos[1])
            self.schedule_level = int(model_infos[2])
            self.ref_iterations = int(model_infos[3])
            self.best_win_rate = float(model_infos[4])
        self.data_helper.set_agents_meta(level=self.schedule_level)

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


def main():
    try:
        pipeline = TrainingPipeline(TRAINING_CONFIG["model_file"])
        pipeline.run()
    except KeyboardInterrupt:
        pipeline.save_model("current_model", rich_info=True)
        print("\nQuit")


if __name__ == "__main__":
    main()
