#include "DeepQLearning.hpp"

//#include <iostream>

DeepQLearning::DeepQLearning(unsigned int input_x, unsigned int input_y, unsigned int actions, unsigned int exploration_action_start, unsigned int exploration_action_end, size_t experience_replay_buffer_size, bool use_gpu, const std::string &graph_filename, const std::string &checkpoint_filename) : input_x(input_x), input_y(input_y), actions(actions), exploration_action_start(exploration_action_start), exploration_action_end(exploration_action_end), experience_replay_buffer_size(experience_replay_buffer_size) {
	experience_replay_buffer.resize(experience_replay_buffer_size);

	// Create session
	tensorflow::SessionOptions options;
	tensorflow::ConfigProto* config = &options.config;
	if(!use_gpu) {
		//std::cout << "Disable GPU!" << std::endl;
		(*config->mutable_device_count())["GPU"] = 0;
	} else {
		config->mutable_gpu_options()->set_allow_growth(true);
		//std::cout << "Use GPU!" << std::endl;
	}
	TF_CHECK_OK(tensorflow::NewSession(options, &sess));

	// Load model
	TF_CHECK_OK(LoadModel(sess, graph_filename, checkpoint_filename));
}

tensorflow::Status DeepQLearning::LoadModel(tensorflow::Session *sess, const std::string &graph_fn, const std::string &checkpoint_fn) {
	tensorflow::Status status;

	// Read in the protobuf graph we exported
	tensorflow::MetaGraphDef graph_def;
	status = ReadBinaryProto(tensorflow::Env::Default(), graph_fn, &graph_def);
	if(status != tensorflow::Status::OK()) {
		return status;
	}

	// Create the graph
	status = sess->Create(graph_def.graph_def());
	if(status != tensorflow::Status::OK()) {
		return status;
	}

	// Restore model from checkpoint, iff checkpoint is given
	if(checkpoint_fn != "") {
		tensorflow::Tensor checkpointPathTensor(tensorflow::DT_STRING, tensorflow::TensorShape());
		checkpointPathTensor.scalar<tensorflow::tstring>()() = checkpoint_fn;

		tensor_dict feed_dict = {{graph_def.saver_def().filename_tensor_name(), checkpointPathTensor}};
		status = sess->Run(feed_dict, {}, {graph_def.saver_def().restore_op_name()}, nullptr);
		if(status != tensorflow::Status::OK()) {
			return status;
		}
	} else {
		status = sess->Run({}, {}, {"init"}, nullptr);
		if(status != tensorflow::Status::OK()) {
			return status;
		}
	}

	return tensorflow::Status::OK();
}

tensorflow::int64 DeepQLearning::getPrediction(const tensorflow::Tensor &observation_tf, double epsilon) {
	tensorflow::int64 action;

	std::bernoulli_distribution greedy_distribution(epsilon);
	if(greedy_distribution(random_generator)) {
		// Exploration
		std::uniform_int_distribution<tensorflow::int64> channel_distribution(exploration_action_start, exploration_action_end);
		action = channel_distribution(random_generator);
	} else {
		// Exploitation
		std::vector<tensorflow::Tensor> outputs;
		TF_CHECK_OK(sess->Run({{"inputs", observation_tf}}, {"predict"}, {}, &outputs));
		action = outputs[0].flat<tensorflow::int64>()(0);
	}

	return action;
}

void DeepQLearning::addExperience(const tensorflow::Tensor &observation_tf, const tensorflow::int64 &action, const tensorflow::int8 &reward, const tensorflow::Tensor &observation_new_tf) {
	std::unique_lock<std::mutex> experience_replay_buffer_lock(experience_replay_buffer_mutex);

	// Fill experience replay buffer
	experience_replay_buffer[experience_replay_buffer_next_id] = Experience(tensorflow::tensor::DeepCopy(observation_tf), action, reward, tensorflow::tensor::DeepCopy(observation_new_tf));

	if(experience_replay_buffer_next_id >= experience_replay_buffer_max_id) {
		experience_replay_buffer_max_id = experience_replay_buffer_next_id;
	}
	experience_replay_buffer_next_id = (experience_replay_buffer_next_id + 1) % (experience_replay_buffer_size - 1);
}

bool DeepQLearning::train(double discount_factor, size_t experience_replay_batch_size) {
	std::unique_lock<std::mutex> experience_replay_buffer_lock(experience_replay_buffer_mutex);

	//std::cout << experience_replay_buffer_max_id << std::endl;

	if(experience_replay_buffer_max_id + 1 >= experience_replay_batch_size) {
		tensorflow::TensorShape observation_batch_shape({(unsigned int) experience_replay_batch_size, input_y, input_x});
		tensorflow::Tensor observation_batch_tf(tensorflow::DT_FLOAT, observation_batch_shape);
		auto observation_batch = observation_batch_tf.tensor<float, 3>();
		tensorflow::Tensor observation_new_batch_tf(tensorflow::DT_FLOAT, observation_batch_shape);
		auto observation_new_batch = observation_new_batch_tf.tensor<float, 3>();

		std::vector<tensorflow::int64> action_batch(experience_replay_batch_size);
		std::vector<tensorflow::int8> reward_batch(experience_replay_batch_size);

		for(unsigned int i = 0; i < experience_replay_batch_size; i++) {
			std::uniform_int_distribution<size_t> replay_distribution(0, experience_replay_buffer_max_id);
			Experience const &item = experience_replay_buffer.at(replay_distribution(random_generator));

			auto observation = item.observation_tf.tensor<float, 2>();
			auto observation_new = item.observation_new_tf.tensor<float, 2>();
			for(int y = 0; y < input_y; y++) {
				for(int x = 0; x < input_x; x++) {
					observation_batch(i, y, x) = observation(y, x);
					observation_new_batch(i, y, x) = observation_new(y, x);
				}
			}

			action_batch[i] = item.action;
			reward_batch[i] = item.reward;
		}

		experience_replay_buffer_lock.unlock();

		std::vector<tensorflow::Tensor> Q_old_tf;
		TF_CHECK_OK(sess->Run({{"inputs", observation_batch_tf}}, {"qout/BiasAdd"}, {}, &Q_old_tf));
		auto Q_old = Q_old_tf[0].tensor<float, 3>();

		std::vector<tensorflow::Tensor> Q_new_max_tf;
		TF_CHECK_OK(sess->Run({{"inputs", observation_new_batch_tf}}, {"qout_max"}, {}, &Q_new_max_tf));
		auto Q_new_max = Q_new_max_tf[0].tensor<float, 2>();

		for(unsigned int i = 0; i < experience_replay_batch_size; i++) {
			Q_old(i, 0, action_batch.at(i)) = reward_batch.at(i) + discount_factor * Q_new_max(i, 0);
			//std::cout << action_batch.at(i) << ": " << (int) reward_batch.at(i) << " + " << discount_factor << " * " << Q_new_max(i, 0) << " = " <<  Q_old(i, 0, action_batch.at(i)) << std::endl;
		}
		TF_CHECK_OK(sess->Run({{"inputs", observation_batch_tf}, {"nextq", Q_old_tf[0]}}, {}, {"updatemodel"}, nullptr));

		return true;
	}

	return false;
}
