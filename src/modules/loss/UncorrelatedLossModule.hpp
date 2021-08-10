#ifndef UNCORRELATED_LOSS_MODULE_HPP
#define UNCORRELATED_LOSS_MODULE_HPP

#include <cstdint>
#include <random>
#include <atomic>
#include <memory>

#include <boost/asio.hpp>

#include "../Module.hpp"
#include "../../utils/Packet.hpp"

class UncorrelatedLossModule : public Module {
	public:
		UncorrelatedLossModule(double loss, uint32_t seed = 1);

		const char* getType() const {
			return "uncorrelated_loss";
		}

	private:
		ReceivingPort<std::shared_ptr<Packet>> input_port_lr;
		SendingPort<std::shared_ptr<Packet>> output_port_lr;
		ReceivingPort<std::shared_ptr<Packet>> input_port_rl;
		SendingPort<std::shared_ptr<Packet>> output_port_rl;

		Parameter parameter_loss = {10, 0, 100, 1};
		Parameter parameter_seed = {1, 0, std::numeric_limits<double>::quiet_NaN(), 1};

		std::mt19937 generator_loss;
		std::unique_ptr<std::bernoulli_distribution> distribution;

		void receiveFromLeftModule(std::shared_ptr<Packet> packet);
		void receiveFromRightModule(std::shared_ptr<Packet> packet);
};

#endif
