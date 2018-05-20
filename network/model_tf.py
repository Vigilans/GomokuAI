from core import GameConfig, Board
import tensorflow as tf
import numpy as np


class PolicyValueNetwork:
    def __init__(self, model_file=None):
        with tf.variable_scope("InputLayer"):
            input_shape = Board().encoded_states().shape  # (6, 15, 15)
            self.inputs = tf.placeholder(tf.float32, (None, *input_shape), "inputs")

        with tf.variable_scope("SharedNet"):
            conv_blocks = []
            for i in range(3):
                conv_blocks.append(
                    tf.layers.conv2d(
                        inputs=self.inputs if i == 0 else conv_blocks[i - 1],
                        filters=2**(i + 5),  # 32, 64, 128
                        kernel_size=(3, 3),
                        padding="same",
                        data_format="channels_first",
                        activation=tf.nn.relu,
                        name="conv {}".format(i)
                    )
                )
            shared_output = conv_blocks[-1]

        with tf.variable_scope("PolicyHead"):
            policy_conv = tf.layers.conv2d(
                inputs=shared_output,
                filters=4,
                kernel_size=(1, 1),
                padding="same",
                data_format="channels_first",
                activation=tf.nn.relu
            )
            policy_flatten = tf.layers.flatten(policy_conv)
            policy_logits = tf.layers.dense(policy_flatten, GameConfig.board_size)
            self.policy_output = tf.nn.softmax(policy_logits)

        with tf.variable_scope("ValueHead"):
            value_conv = tf.layers.conv2d(
                inputs=shared_output,
                filters=2,
                kernel_size=(1, 1),
                padding="same",
                data_format="channels_first",
                activation=tf.nn.relu
            )
            value_flatten = tf.layers.flatten(value_conv)
            value_hidden = tf.layers.dense(value_flatten, 64, tf.nn.relu)
            self.value_output = tf.layers.dense(value_hidden, 1, tf.nn.relu)

        with tf.variable_scope("Loss-Entropy"):
            # labels
            self.state_value = tf.placeholder(tf.float32, (None, 1), "state_value")
            self.action_probs = tf.placeholder(tf.float32, (None, GameConfig.board_size), "action_probs")
            # losses
            value_loss = tf.losses.mean_squared_error(self.state_value, self.value_output)
            policy_loss = tf.losses.softmax_cross_entropy(self.action_probs, policy_logits)
            l2_reg = tf.contrib.layers.apply_regularization(
                tf.contrib.layers.l2_regularizer(1e-4),
                tf.trainable_variables()  # TODO: check whether...
            )
            # final loss
            self.loss = value_loss + policy_loss + l2_reg
            self.entropy = tf.reduce_mean(
                -tf.reduce_sum(self.policy_output * tf.log(self.policy_output), 1)
            )

        self.session = tf.session()
        self.session.run(tf.global_variables_initializer())
        self.saver = tf.train.saver()

        if model_file is not None:
            self.restore_model(model_file)

    def compile(self, opt):
        """
        Optimization definition
        """
        with tf.variable_scope("Optimizer"):
            self.lr = tf.placeholder(tf.float32, "learning_rate")
            self.opt = tf.train.AdamOptimizer(self.lr).minimize(self.loss)

    def eval_state(self, state):
        """
        Evaluate a board state.
        """
        value, probs = self.session.run(
            [self.value_output, self.policy_output],
            feed_dict={self.inputs: state.encoded_states()[np.newaxis, :]}
        )
        return value, probs

    def build_dataset(self, generator):
        self.dataset = tf.data.Dataset.from_generator(
            generator,
            (tf.int8, (tf.float32, tf.float32)),
            (self.input.shape, (self.value_output.shape, self.policy_output.shape))
        )
        self.dataset.prefetch(2)

    def train_step(self, inputs, winner, probs, opt):
        """
        One Network Tranning step.
        """
        loss, entropy, _ = self.session.run(
            [self.loss, self.entropy, self.opt],
            feed_dict={
                self.inputs: inputs,
                self.state_value: winner,
                self.action_probs: probs,
                self.lr: opt["lr"]
            }
        )
        return loss, entropy

    def save_model(self, model_path):
        self.saver.save(self.session, model_path)

    def restore_model(self, model_path):
        self.saver.restore(self.session, model_path)
