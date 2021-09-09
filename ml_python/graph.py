# FlowEmu - Flow-Based Network Emulator
# Copyright (c) 2021 Institute of Communication Networks (ComNets),
#                    Hamburg University of Technology (TUHH),
#                    https://www.tuhh.de/comnets
# Copyright (c) 2021 Daniel Stolpmann <daniel.stolpmann@tuhh.de>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

import tensorflow.compat.v1 as tf
tf.disable_v2_behavior()

def create(input_x, input_y, outputs):
    tf.reset_default_graph()

    # These lines establish the feed-forward part of the network used to choose actions
    inputs = tf.placeholder(shape=(None, input_x, input_y), dtype=tf.float32, name='inputs')
    #float_inputs = tf.cast(inputs, dtype=tf.float32)
    flat_inputs = tf.reshape(inputs, [-1, 1, input_x * input_y])
    Qhidden1 = tf.layers.dense(flat_inputs, units=64, activation=tf.nn.relu)
    Qhidden2 = tf.layers.dense(Qhidden1, units=64, activation=tf.nn.relu)
    Qout = tf.layers.dense(Qhidden2, units=(outputs), name='qout')
    Qout_max = tf.reduce_max(Qout, axis=2, name='qout_max')
    predict = tf.argmax(Qout, 2, name='predict')

    # Below we obtain the loss by taking the sum of squared differences between the target and predicted Q values
    nextQ = tf.placeholder(shape=[None, 1, (outputs)], dtype=tf.float32, name='nextq')
    loss = tf.reduce_sum(tf.squared_difference(nextQ, Qout))
    trainer = tf.train.AdamOptimizer(learning_rate=0.001, epsilon=0.1)
    updateModel = trainer.minimize(loss, name='updatemodel')

    init = tf.initializers.global_variables()

    return [init, updateModel, predict, inputs, Qout, Qout_max, nextQ]
