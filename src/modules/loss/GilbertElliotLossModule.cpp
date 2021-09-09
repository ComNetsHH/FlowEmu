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
 */

#include "GilbertElliotLossModule.hpp"

//#include <iostream>

#include <boost/bind.hpp>

using namespace std;

GilbertElliotLossModule::GilbertElliotLossModule(boost::asio::io_service &io_service, double p01, double p10, double e0, double e1, uint32_t seed_transition, uint32_t seed_loss) : timer_transition(io_service) {
	setName("Gilbert-Elliot Loss");
	addPort({"lr_in", "In", PortInfo::Side::left, &input_port_lr});
	addPort({"lr_out", "Out", PortInfo::Side::right, &output_port_lr});
	addPort({"rl_in", "In", PortInfo::Side::right, &input_port_rl});
	addPort({"rl_out", "Out", PortInfo::Side::left, &output_port_rl});
	addParameter({"p01", "p_01", "1/ms", &parameter_p01});
	addParameter({"p10", "p_10", "1/ms", &parameter_p10});
	addParameter({"e0", "e_0", "%", &parameter_e0});
	addParameter({"e1", "e_1", "%", &parameter_e1});
	addParameter({"seed_transition", "Transition Seed", "", &parameter_seed_transition});
	addParameter({"seed_loss", "Loss Seed", "", &parameter_seed_loss});
	addStatistic({"state", "State", "", &statistic_state});

	input_port_lr.setReceiveHandler(bind(&GilbertElliotLossModule::receiveFromLeftModule, this, placeholders::_1));
	input_port_rl.setReceiveHandler(bind(&GilbertElliotLossModule::receiveFromRightModule, this, placeholders::_1));

	parameter_p01.addChangeHandler([&](double value) {
		distribution_p01.reset(new exponential_distribution<double>(value));
	});
	parameter_p10.addChangeHandler([&](double value) {
		distribution_p10.reset(new exponential_distribution<double>(value));
	});
	parameter_e0.addChangeHandler([&](double value) {
		distribution_e0.reset(new bernoulli_distribution(value / 100));
	});
	parameter_e1.addChangeHandler([&](double value) {
		distribution_e1.reset(new bernoulli_distribution(value / 100));
	});

	parameter_p01.set(p01);
	parameter_p10.set(p10);
	parameter_e0.set(e0);
	parameter_e1.set(e1);
	parameter_seed_transition.set(seed_transition);
	parameter_seed_loss.set(seed_loss);

	parameter_seed_transition.addChangeHandler(bind(&GilbertElliotLossModule::reset, this));
	parameter_seed_loss.addChangeHandler(bind(&GilbertElliotLossModule::reset, this));

	reset();
}

void GilbertElliotLossModule::reset() {
	timer_transition.cancel();

	generator_transition.seed(parameter_seed_transition.get());
	generator_loss.seed(parameter_seed_loss.get());
	state = 0;

	chrono::high_resolution_clock::duration sojourn_time = chrono::nanoseconds((uint64_t) ((*distribution_p01)(generator_transition) * 1000000));
	//cout << "Gilbert-Elliot model: Stay in state 0 with " << parameter_e0.get() << "\% loss for " << (double) sojourn_time.count() / 1000000 << "ms" << endl;
	timer_transition.expires_from_now(sojourn_time);
	timer_transition.async_wait(boost::bind(&GilbertElliotLossModule::transition, this, boost::asio::placeholders::error));

	statistic_state.set(state);
}

void GilbertElliotLossModule::transition(const boost::system::error_code& error) {
	if(error == boost::asio::error::operation_aborted) {
		return;
	}

	if(state == 0) {
		state = 1;

		chrono::high_resolution_clock::duration sojourn_time = chrono::nanoseconds((uint64_t) ((*distribution_p10)(generator_transition) * 1000000));
		//cout << "Gilbert-Elliot model: Stay in state 1 with " << parameter_e1.get() << "\% loss for " << (double) sojourn_time.count() / 1000000 << "ms" << endl;
		timer_transition.expires_at(timer_transition.expiry() + sojourn_time);
	} else {
		state = 0;

		chrono::high_resolution_clock::duration sojourn_time = chrono::nanoseconds((uint64_t) ((*distribution_p01)(generator_transition) * 1000000));
		//cout << "Gilbert-Elliot model: Stay in state 0 with " << parameter_e0.get() << "\% loss for " << (double) sojourn_time.count() / 1000000 << "ms" << endl;
		timer_transition.expires_at(timer_transition.expiry() + sojourn_time);
	}

	timer_transition.async_wait(boost::bind(&GilbertElliotLossModule::transition, this, boost::asio::placeholders::error));

	statistic_state.set(state);
}

bool GilbertElliotLossModule::isLost() {
	if(state == 0) {
		return (*distribution_e0)(generator_loss);
	} else {
		return (*distribution_e1)(generator_loss);
	}
}

void GilbertElliotLossModule::receiveFromLeftModule(shared_ptr<Packet> packet) {
	if(!isLost()) {
		output_port_lr.send(packet);
	}
}

void GilbertElliotLossModule::receiveFromRightModule(shared_ptr<Packet> packet) {
	if(!isLost()) {
		output_port_rl.send(packet);
	}
}

GilbertElliotLossModule::~GilbertElliotLossModule() {
	timer_transition.cancel();
}
