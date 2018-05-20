from core import GameConfig, Board
import tensorflow as tf
import numpy as np


class PolicyValueNetwork:
    def __init__(self, model_file=None):
        with tf.variable_scope("Dataset"):
            input_shape = Board().encoded_states().shape  # (6, 15, 15)
            self.iter = tf.data.Iterator.from_structure(
                (tf.int8, tf.float32, tf.float32),
                (
                    tf.TensorShape((None, *input_shape)),
                    tf.TensorShape((None, )),
                    tf.TensorShape((None, GameConfig.board_size))
                )
            )
            self.inputs, self.state_value, self.policy_output = self.iter.get_next()

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
            self.policy_logits = tf.layers.dense(policy_flatten, GameConfig.board_size)
            self.policy_output = tf.nn.softmax(self.policy_logits)

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

        self.session = tf.session()
        self.session.run(tf.global_variables_initializer())
        self.saver = tf.train.saver()

        if model_file is not None:
            self.restore_model(model_file)

    def compile(self, opt):
        """
        Loss/Entropy and Optimization definition
        """
        with tf.variable_scope("Loss-Entropy"):
            # losses
            value_loss = tf.losses.mean_squared_error(self.state_value, self.value_output)
            policy_loss = tf.losses.softmax_cross_entropy(self.action_probs, self.policy_logits)
            l2_reg = tf.contrib.layers.apply_regularization(
                tf.contrib.layers.l2_regularizer(1e-4),
                tf.trainable_variables()  # TODO: check whether...
            )
            # final loss
            self.loss = value_loss + policy_loss + l2_reg
            self.entropy = tf.reduce_mean(
                -tf.reduce_sum(self.policy_output * tf.log(self.policy_output), 1)
            )

        with tf.variable_scope("Optimizer"):
            self.lr = tf.placeholder(tf.float32, "learning_rate")
            self.opt = tf.train.AdamOptimizer(self.lr).minimize(self.loss)

    def build_dataset(self, generator, num_epoches):
        """
        Generator output:
        ([state_batch, value_batch, policy_batch]*num_epoches...)
        """
        placeholders = [self.input, self.value_output, self.policy_output]
        self.dataset = tf.data.Dataset.from_generator(
            generator,
            [data.dtype for data in placeholders],
            [data.shape for data in placeholders]
        )
        self.dataset = self.dataset.prefetch(2*num_epoches)
        self.session.run(self.iter.make_initializer(self.dataset))

    def train_step(self, optimizer, num_epoches):
        """
        Multiple epoches of network training steps.
        """
        old_probs = None  # will be assigned later
        for i in range(num_epoches):
            new_probs, loss, entropy, _ = self.session.run(
                [self.policy_output, self.loss, self.entropy, self.opt],
                feed_dict={self.lr: optimizer["lr"]*optimizer["lr_multiplier"]}
            )

            if i == 0:
                old_probs = new_probs
                kl = 0  # KL divergence is apparently zero
            else:
                kl = tf.reduce_mean(
                    tf.reduce_sum(old_probs * tf.log(old_probs / new_probs), 1)
                )

            # early stop when KL diverges too much
            if kl > 4 * optimizer["kl_target"]:
                self.dataset = self.dataset.skip(num_epoches - i)
                print("Early stop and skip {} elems".format(num_epoches - i))
                break

        return loss, entropy, kl

    def eval_state(self, state):
        """
        Evaluate a board state.
        """
        value, probs = self.session.run(
            [self.value_output, self.policy_output],
            feed_dict={self.inputs: state.encoded_states()[np.newaxis, :]}
        )
        return value, probs

    def save_model(self, model_path):
        self.saver.save(self.session, model_path)

    def restore_model(self, model_path):
        self.saver.restore(self.session, model_path)
