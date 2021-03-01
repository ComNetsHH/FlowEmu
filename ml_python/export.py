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

