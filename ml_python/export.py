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

import graph

def exportModel(path, input_x, input_y, outputs):
    init, updateModel, predict, inputs, Qout, Qout_max, nextQ = graph.create(input_x, input_y, outputs)

    with tf.Session() as sess:
        sess.run(init)

        print("Node names:")
        print("updateModel: " + updateModel.name)
        print("predict: " + predict.name)
        print("inputs: " + inputs.name)
        print("Qout: " + Qout.name)
        print("Qout_max: " + Qout_max.name)
        print("nextQ: " + nextQ.name)

        saver = tf.train.Saver(tf.global_variables())
        saver.save(sess, path + '/graph')
        tf.train.write_graph(sess.graph, path, 'graph.pb', as_text=False)

if __name__ == "__main__":
    exportModel('../ml_models', 10, 3, 2)

