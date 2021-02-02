#ifndef GILBERT_ELLIOT_LOSS_MODULE_HPP
#define GILBERT_ELLIOT_LOSS_MODULE_HPP

#include <cstdint>
#include <random>
#include <atomic>
#include <memory>

#include <boost/asio.hpp>

#include "../Module.hpp"
#include "../../utils/Mqtt.hpp"
#include "../../utils/Packet.hpp"

class GilbertElliotLossModule : public ModuleHasLeft<std::shared_ptr<Packet>>, public ModuleHasRight<std::shared_ptr<Packet>> {
	public:
		GilbertElliotLossModule(boost::asio::io_service &io_service, Mqtt &mqtt, double p01, double p10, double e0 = 0, double e1 = 1, uint32_t seed_transition = 1, uint32_t seed_loss = 1);

		void setModelParameters(double p01, double p10, double e0 = 0, double e1 = 1);
		void setSeedTransition(uint32_t seed_transition);
		void setSeedLoss(uint32_t seed_loss);

		void reset();

		void receiveFromLeftModule(std::shared_ptr<Packet> packet) override;
		void receiveFromRightModule(std::shared_ptr<Packet> packet) override;
	private:
		Mqtt &mqtt;

		std::atomic<double> p01;
		std::atomic<double> p10;
		std::atomic<double> e0;
		std::atomic<double> e1;
		std::atomic<uint32_t> seed_transition;
		std::atomic<uint32_t> seed_loss;
		std::atomic<bool> state;

		std::default_random_engine generator_transition;
		std::default_random_engine generator_loss;
		std::unique_ptr<std::exponential_distribution<double>> distribution_p01;
		std::unique_ptr<std::exponential_distribution<double>> distribution_p10;
		std::unique_ptr<std::bernoulli_distribution> distribution_e0;
		std::unique_ptr<std::bernoulli_distribution> distribution_e1;

		boost::asio::high_resolution_timer timer_transition;
		void transition(const boost::system::error_code& error);
		bool isLost();
};

#endif
