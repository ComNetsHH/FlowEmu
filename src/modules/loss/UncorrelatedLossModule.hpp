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

class UncorrelatedLossModule : public ModuleHasLeft<std::shared_ptr<Packet>>, public ModuleHasRight<std::shared_ptr<Packet>> {
	public:
		UncorrelatedLossModule(Mqtt &mqtt, double p, uint32_t seed = 1);

		void setLossProbability(double p);
		void setSeed(uint32_t seed);

		void receiveFromLeftModule(std::shared_ptr<Packet> packet) override;
		void receiveFromRightModule(std::shared_ptr<Packet> packet) override;
	private:
		Mqtt &mqtt;

		std::atomic<double> p;
		std::atomic<uint32_t> seed;

		std::default_random_engine generator_loss;
		std::unique_ptr<std::bernoulli_distribution> distribution;
};

#endif
