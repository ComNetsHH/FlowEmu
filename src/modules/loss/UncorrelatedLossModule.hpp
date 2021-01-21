#ifndef UNCORRELATED_LOSS_MODULE_HPP
#define UNCORRELATED_LOSS_MODULE_HPP

#include <vector>
#include <cstdint>
#include <random>

#include <boost/asio.hpp>

#include "../Module.hpp"

class UncorrelatedLossModule : public ModuleHasLeft<boost::asio::const_buffer>, public ModuleHasRight<boost::asio::const_buffer> {
	public:
		UncorrelatedLossModule(double p, uint32_t seed_loss = 1);

		void receiveFromLeftModule(boost::asio::const_buffer packet) override;
		void receiveFromRightModule(boost::asio::const_buffer packet) override;
	private:
		std::default_random_engine generator_loss;
		std::bernoulli_distribution distribution;
};

#endif
