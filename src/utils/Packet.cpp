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

#include "Packet.hpp"

#include <iostream>
#include <ios>

using namespace std;

Packet::Packet(const vector<uint8_t> &bytes) : bytes(bytes) {
}

Packet::Packet(const uint8_t *data, size_t size) : bytes(data, data + size) {
}

void Packet::setBytes(const vector<uint8_t> &bytes) {
	this->bytes = bytes;
}

const vector<uint8_t>& Packet::getBytes() const {
	return bytes;
}

const chrono::high_resolution_clock::time_point& Packet::getCreationTimePoint() const {
	return creation_time_point;
}