from core import GameConfig as Game
from core import Board
from config import TRAINING_CONFIG
from keras import Sequential, Model, Input
from keras.layers import InputLayer
from keras.layers.core import Activation, Dense, Flatten
from keras.layers.convolutional import Conv2D
from keras.layers.merge import Add
from keras.layers.normalization import BatchNormalization
from keras.optimizers import sgd
from keras.regularizers import l2
from keras import backend as K
import numpy as np
import os


def ConvBlock(
        filter_size=256,
        kernel_size=(3, 3),
        activation=None,
        input_shape=None
    ) -> list:
    """
    Conv block with no activation func.
    if activation is None,
    then Activation layer will not be added
    """
    return [
        *([InputLayer(input_shape)] if input_shape else []),
        Conv2D(
            filters=filter_size,
            kernel_size=kernel_size,
            padding="same",
            data_format="channels_first",
            kernel_regularizer=l2()
        ),
        BatchNormalization(epsilon=1e-5),
        *([Activation(activation)] if activation else [])
    ]


# def ResBlock(identity_input) -> list:
#     """ Residual Conv block """
#     return Sequential([
#         Add()([
#             identity_input,
#             Sequential([

#             ]),
#         ]),
#         Activation("relu")
#     ])


class PolicyValueNetwork:
    """ AlphaZero Residual-CNN """
    def __init__(self, model_file=None):

        # Build Network Architecture
        input_shape = Board().encoded_states().shape  # (6, 15, 15)
        inputs = Input(input_shape)

        shared_net = Sequential([
            *ConvBlock(32, input_shape=input_shape),
            *ConvBlock(64),
            *ConvBlock(128)
        ], "shared_net")

        policy_head = Sequential([
            shared_net,
            *ConvBlock(4, (1, 1), "relu"),
            Flatten(),
            Dense(Game["board_size"], kernel_regularizer=l2()),
            Activation("softmax")
        ], "policy_head")

        value_head = Sequential([
            shared_net,
            *ConvBlock(2, (1, 1), "relu"),
            Flatten(),
            Dense(64, activation="relu", kernel_regularizer=l2()),
            Dense(1, kernel_regularizer=l2()),
            Activation("tanh")
        ], "value_head")

        self.model = Model(
            inputs,
            [value_head(inputs), policy_head(inputs)]
        )

        if model_file is not None:
            self.restore_model(model_file)

    def compile(self, opt):
        """
        Optimization and Loss definition
        """
        self.model.compile(
            optimizer=sgd(),
            loss=["mse", "categorical_crossentropy"]
        )

    def eval_state(self, state):
        """
        Evaluate a board state.
        """
        vp = self.model.predict_on_batch(state.encoded_states()[np.newaxis, :])
        # format to (float, np.array((255,1),dtype=float)) structure
        return vp[0][0][0], vp[1][0]

    def train_step(self, optimizer):
        """
        One Network Tranning step.
        """
        opt = self.model.optimizer
        K.set_value(opt.lr, optimizer["lr"])
        K.set_value(opt.momentum, optimizer["momentum"])
        # loss = self.model.train_on_batch(inputs, [winner, probs])
        # return loss

    def save_model(self, filename):
        base_path = "{}/keras".format(TRAINING_CONFIG["model_path"])
        if not os.path.exists(base_path):
            os.mkdir(base_path)
        self.model.save_weights("{}/{}.h5".format(base_path, filename))

    def restore_model(self, filename):
        base_path = "{}/keras".format(TRAINING_CONFIG["model_path"])
        if os.path.exists("{}/{}.h5".format(base_path, filename)):
            self.model.load_weights("{}/{}.h5".format(base_path, filename))
