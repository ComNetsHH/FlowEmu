#include "FixedIntervalRateModule.hpp"

#include <boost/bind.hpp>

using namespace std;

FixedIntervalRateModule::FixedIntervalRateModule(boost::asio::io_service &io_service, chrono::high_resolution_clock::duration interval) : timer_lr(io_service), timer_rl(io_service) {
	setName("Fixed Interval Rate");
	addPort({"lr_in", "In", PortInfo::Side::left, &input_port_lr});
	addPort({"lr_out", "Out", PortInfo::Side::right, &output_port_lr});
	addPort({"rl_in", "In", PortInfo::Side::right, &input_port_rl});
	addPort({"rl_out", "Out", PortInfo::Side::left, &output_port_rl});
	addParameter({"interval", "Interval", "ms", &parameter_interval});
	addParameter({"rate", "Rate", "packets/s", &parameter_rate});

	parameter_interval.addChangeHandler([&](double value) {
		double rate = parameter_rate.get();
		double rate_from_interval = 1000 / value;

		if(rate != rate_from_interval) {
			parameter_rate.set(rate_from_interval);
		}
	});
	parameter_rate.addChangeHandler([&](double value) {
		double interval = parameter_interval.get();
		double interval_from_rate = 1000 / value;

		if(interval != interval_from_rate) {
			parameter_interval.set(interval_from_rate);
		}
	});

	parameter_interval.set((double) chrono::nanoseconds(interval).count() / 1000000);

	chrono::high_resolution_clock::time_point now = chrono::high_resolution_clock::now();

	timer_lr.expires_at(now);
	timer_lr.async_wait(boost::bind(&FixedIntervalRateModule::processLr, this, boost::asio::placeholders::error));

	timer_rl.expires_at(now);
	timer_rl.async_wait(boost::bind(&FixedIntervalRateModule::processRl, this, boost::asio::placeholders::error));
}

void FixedIntervalRateModule::processLr(const boost::system::error_code& error) {
	if(error == boost::asio::error::operation_aborted) {
		return;
	}

	auto packet = input_port_lr.request();
	if(packet != nullptr) {
		output_port_lr.send(packet);
	}

	timer_lr.expires_at(timer_lr.expiry() + chrono::nanoseconds((uint64_t) (parameter_interval.get() * 1000000)));
	timer_lr.async_wait(boost::bind(&FixedIntervalRateModule::processLr, this, boost::asio::placeholders::error));
}

void FixedIntervalRateModule::processRl(const boost::system::error_code& error) {
	if(error == boost::asio::error::operation_aborted) {
		return;
	}

	auto packet = input_port_rl.request();
	if(packet != nullptr) {
		output_port_rl.send(packet);
	}

	timer_rl.expires_at(timer_rl.expiry() + chrono::nanoseconds((uint64_t) (parameter_interval.get() * 1000000)));
	timer_rl.async_wait(boost::bind(&FixedIntervalRateModule::processRl, this, boost::asio::placeholders::error));
}

FixedIntervalRateModule::~FixedIntervalRateModule() {
	timer_lr.cancel();
	timer_rl.cancel();
}
