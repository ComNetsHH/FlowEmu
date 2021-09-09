/*
 * FlowEmu - Flow-Based Network Emulator
 * Copyright (c) 2021 Institute of Communication Networks (ComNets),
 *                    Hamburg University of Technology (TUHH),
 *                    https://www.tuhh.de/comnets
 * Copyright (c) 2021 Daniel Stolpmann <daniel.stolpmann@tuhh.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *     Copyright (c) 2018 Patrick Wieschollek <mail@patwie.com>
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

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

		std::mt19937 random_generator;
	public:
		DeepQLearning(unsigned int input_x, unsigned int input_y, unsigned int actions, unsigned int exploration_action_start, unsigned int exploration_action_end, size_t experience_replay_buffer_size = 1000000, bool use_gpu = false, const std::string &graph_filename = "graph.meta", const std::string &checkpoint_filename = "graph");

		tensorflow::int64 getPrediction(const tensorflow::Tensor &observation_tf, double epsilon);

		void addExperience(const tensorflow::Tensor &observation_tf, const tensorflow::int64 &action, const tensorflow::int8 &reward, const tensorflow::Tensor &observation_new_tf);
		bool train(double discount_factor, size_t experience_replay_batch_size = 32);
};

#endif
