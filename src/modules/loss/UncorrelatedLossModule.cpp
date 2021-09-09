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

#include "UncorrelatedLossModule.hpp"

using namespace std;

UncorrelatedLossModule::UncorrelatedLossModule(double loss, uint32_t seed) {
	setName("Uncorrelated Loss");
	addPort({"lr_in", "In", PortInfo::Side::left, &input_port_lr});
	addPort({"lr_out", "Out", PortInfo::Side::right, &output_port_lr});
	addPort({"rl_in", "In", PortInfo::Side::right, &input_port_rl});
	addPort({"rl_out", "Out", PortInfo::Side::left, &output_port_rl});
	addParameter({"loss", "Loss", "%", &parameter_loss});
	addParameter({"seed", "Seed", "", &parameter_seed});

	input_port_lr.setReceiveHandler(bind(&UncorrelatedLossModule::receiveFromLeftModule, this, placeholders::_1));
	input_port_rl.setReceiveHandler(bind(&UncorrelatedLossModule::receiveFromRightModule, this, placeholders::_1));

	parameter_loss.addChangeHandler([&](double value) {
		distribution.reset(new bernoulli_distribution(value / 100));
	});
	parameter_seed.addChangeHandler([&](double value) {
		generator_loss.seed(value);
	});

	parameter_loss.set(loss);
	parameter_seed.set(seed);
}

void UncorrelatedLossModule::receiveFromLeftModule(shared_ptr<Packet> packet) {
	if(!(*distribution)(generator_loss)) {
		output_port_lr.send(packet);
	}
}

void UncorrelatedLossModule::receiveFromRightModule(shared_ptr<Packet> packet) {
	if(!(*distribution)(generator_loss)) {
		output_port_rl.send(packet);
	}
}
