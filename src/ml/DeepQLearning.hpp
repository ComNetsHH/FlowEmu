// Based on: https://github.com/PatWie/tensorflow-cmake/blob/master/inference/cc/inference_cc.cc
// Inspired by: https://medium.com/emergent-future/simple-reinforcement-learning-with-tensorflow-part-0-q-learning-with-tables-and-neural-networks-d195264329d0
// Inspired by: https://github.com/dennybritz/reinforcement-learning/blob/master/DQN/Deep%20Q%20Learning%20Solution.ipynb

#ifndef DEEP_Q_LEARNING_HPP
#define DEEP_Q_LEARNING_HPP

#include <string>
#include <list>
#include <memory>
#include <random>
#include <mutex>

#include <tensorflow/core/protobuf/meta_graph.pb.h>
#include <tensorflow/core/public/session.h>
#include <tensorflow/core/public/session_options.h>
#include <tensorflow/core/framework/tensor_util.h>

class DeepQLearning {
	private:
		typedef std::vector<std::pair<std::string, tensorflow::Tensor>> tensor_dict;

		tensorflow::Status LoadModel(tensorflow::Session *sess, const std::string &graph_fn, const std::string &checkpoint_fn = "");

		struct Experience {
			Experience() {}

			Experience(tensorflow::Tensor observation_tf, tensorflow::int64 action, tensorflow::int8 reward, tensorflow::Tensor observation_new_tf) :
				observation_tf(observation_tf),
				action(action),
				reward(reward),
				observation_new_tf(observation_new_tf) {
			}

			tensorflow::Tensor observation_tf;
			tensorflow::int64 action;
			tensorflow::int8 reward;
			tensorflow::Tensor observation_new_tf;
		};

		tensorflow::Session *sess;
		unsigned int input_x;
		unsigned int input_y;
		unsigned int actions;
		unsigned int exploration_action_start;
		unsigned int exploration_action_end;

		std::vector<Experience> experience_replay_buffer;
		std::mutex experience_replay_buffer_mutex;
		size_t experience_replay_buffer_size;
		size_t experience_replay_buffer_next_id = 0;
		size_t experience_replay_buffer_max_id = 0;

		std::default_random_engine random_generator;
	public:
		DeepQLearning(unsigned int input_x, unsigned int input_y, unsigned int actions, unsigned int exploration_action_start, unsigned int exploration_action_end, size_t experience_replay_buffer_size = 1000000, bool use_gpu = false, const std::string &graph_filename = "graph.meta", const std::string &checkpoint_filename = "graph");

		tensorflow::int64 getPrediction(const tensorflow::Tensor &observation_tf, double epsilon);

		void addExperience(const tensorflow::Tensor &observation_tf, const tensorflow::int64 &action, const tensorflow::int8 &reward, const tensorflow::Tensor &observation_new_tf);
		bool train(double discount_factor, size_t experience_replay_batch_size = 32);
};

#endif
