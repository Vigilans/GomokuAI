import os

import numpy as np
import tensorflow as tf

from config import TRAINING_CONFIG
from core import GameConfig as Game
from core import Board


class PolicyValueNetwork:
    def __init__(self, model_name=None):
        with tf.variable_scope("Dataset"):
            input_shape = Board().encoded_states().shape  # (6, 15, 15)
            self.iter = tf.data.Iterator.from_structure(
                (tf.float32, tf.float32, tf.float32),
                (
                    tf.TensorShape((None, *input_shape)),
                    tf.TensorShape((None, )),
                    tf.TensorShape((None, Game["board_size"]))
                )
            )
            self.mini_batch = self.iter.get_next()
            self.inputs, self.state_value, self.action_probs = self.mini_batch
            inputs_t = tf.transpose(self.inputs, [0, 2, 3, 1])  # channel_last
            self.inputs_t = inputs_t

        with tf.variable_scope("SharedNet"):
            conv_blocks = []
            for i in range(3):
                conv_blocks.append(
                    tf.layers.conv2d(
                        inputs=inputs_t if i == 0 else conv_blocks[i - 1],
                        filters=2**(i + 5),  # 32, 64, 128
                        kernel_size=(3, 3),
                        padding="same",
                        activation=tf.nn.relu,
                        name="conv_{}".format(i)
                    )
                )
            shared_output = conv_blocks[-1]

        with tf.variable_scope("PolicyHead"):
            policy_conv = tf.layers.conv2d(
                inputs=shared_output,
                filters=4,
                kernel_size=(1, 1),
                padding="same",
                activation=tf.nn.relu
            )
            policy_flatten = tf.layers.flatten(policy_conv)
            self.policy_logits = tf.layers.dense(policy_flatten, Game["board_size"])
            self.policy_output = tf.nn.softmax(self.policy_logits, name="policy_output")

        with tf.variable_scope("ValueHead"):
            value_conv = tf.layers.conv2d(
                inputs=shared_output,
                filters=2,
                kernel_size=(1, 1),
                padding="same",
                activation=tf.nn.relu
            )
            value_flatten = tf.layers.flatten(value_conv)
            value_hidden = tf.layers.dense(value_flatten, 64, tf.nn.relu)
            value_logits = tf.layers.dense(value_hidden, 1)
            self.value_output = tf.reshape(tf.nn.tanh(value_logits), [-1], name="value_output")

        self.session = tf.Session()
        self.saver = tf.train.Saver()
        self.model_file = self._parse_path(model_name)
        self.initialized = False

    def compile(self):
        """
        Loss/Entropy and Optimization definition
        """
        with tf.variable_scope("Loss-Entropy"):
            # losses
            value_loss = tf.losses.mean_squared_error(self.state_value, self.value_output)
            policy_loss = tf.losses.softmax_cross_entropy(self.action_probs, self.policy_logits)
            l2_reg = tf.contrib.layers.apply_regularization(
                tf.contrib.layers.l2_regularizer(1e-4),
                [v for v in tf.trainable_variables() if "bias" not in v.name]
            )
            # final loss
            self.loss = value_loss + policy_loss + l2_reg
            self.entropy = tf.reduce_mean(
                -tf.reduce_sum(self.policy_output * tf.log(self.policy_output + 1e-10), 1)
            )

        with tf.variable_scope("Optimizer"):
            self.lr = tf.placeholder(tf.float32, name="learning_rate")
            self.opt = tf.train.AdamOptimizer(self.lr).minimize(self.loss)

        self._lazy_initialize()

    def build_dataset(self, generator, num_epoches):
        """
        Generator output:
        ([state_batch, value_batch, policy_batch]*num_epoches...)
        """
        placeholders = [self.inputs, self.value_output, self.policy_output]
        self.dataset = tf.data.Dataset.from_generator(
            generator,
            tuple(data.dtype for data in placeholders),
            tuple(data.shape for data in placeholders)
        )
        self.dataset = self.dataset.prefetch(num_epoches)
        self.session.run(self.iter.make_initializer(self.dataset))

    def train_step(self, optimizer, num_epoches):
        """
        Multiple epoches of network training steps.
        """
        old_probs = None  # will be assigned later
        for i in range(num_epoches):
            new_probs, loss, entropy, _ = self.session.run(
                [self.policy_output, self.loss, self.entropy, self.opt],
                feed_dict={self.lr: optimizer["lr"]}
            )

            if i == 0:
                old_probs = new_probs + 1e-10
                kl = 0.0  # KL divergence is apparently zero
            else:
                new_probs += 1e-10
                kl = (old_probs * np.log(old_probs / new_probs)).sum(1).mean()

            # early stop when KL diverges too much
            if kl > 4 * optimizer["kl_target"]:
                for _ in range(num_epoches - 1 - i):
                    self.session.run(self.mini_batch)  # skip redundant data
                break

        return loss, entropy, kl, i + 1

    def eval_state(self, state):
        """
        Evaluate a board state.
        """
        self._lazy_initialize()
        value, probs = self.session.run(
            [self.value_output, self.policy_output],
            feed_dict={self.inputs: state.encoded_states()[np.newaxis, :]}
        )
        return value[0], probs[0]

    def save_model(self, model_name):
        self.saver.save(self.session, self._parse_path(model_name))

    def restore_model(self, model_name):
        model_file = self._parse_path(model_name)
        if model_file is not None:
            self.saver.restore(self.session, model_file)
            self.initialized = True

    def _parse_path(self, model_name):
        if '/' in model_name:
            return model_name  # already parsed
        elif model_name == "latest":
            checkpoint = tf.train.get_checkpoint_state(self._parse_path(""))
            return checkpoint.model_checkpoint_path if checkpoint else None
        elif model_name is not None:
            return f"{TRAINING_CONFIG['model_path']}/tf/{model_name}"
        else:
            return None

    def _lazy_initialize(self):
        if self.initialized:
            return
        if not self.model_file or not os.path.exists(self.model_file):
            self.session.run(tf.global_variables_initializer())
            self.initialized = True
        else:
            self.restore_model(self.model_file)
