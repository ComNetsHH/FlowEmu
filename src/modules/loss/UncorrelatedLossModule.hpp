#ifndef UNCORRELATED_LOSS_MODULE_HPP
#define UNCORRELATED_LOSS_MODULE_HPP

#include <cstdint>
#include <random>
#include <atomic>
#include <memory>

#include <boost/asio.hpp>

#include "../Module.hpp"
#include "../../utils/Mqtt.hpp"
#include "../../utils/Packet.hpp"

class UncorrelatedLossModule : public Module {
	public:
		UncorrelatedLossModule(Mqtt &mqtt, double p, uint32_t seed = 1);

		const char* getType() const {
			return "uncorrelated_loss";
		}

		void setLossProbability(double p);
		void setSeed(uint32_t seed);

		void receiveFromLeftModule(std::shared_ptr<Packet> packet);
		void receiveFromRightModule(std::shared_ptr<Packet> packet);
	private:
		Mqtt &mqtt;

		std::atomic<double> p;
		std::atomic<uint32_t> seed;

		std::default_random_engine generator_loss;
		std::unique_ptr<std::bernoulli_distribution> distribution;

		ReceivingPort<std::shared_ptr<Packet>> input_port_lr;
		SendingPort<std::shared_ptr<Packet>> output_port_lr;

		ReceivingPort<std::shared_ptr<Packet>> input_port_rl;
		SendingPort<std::shared_ptr<Packet>> output_port_rl;
};

#endif
