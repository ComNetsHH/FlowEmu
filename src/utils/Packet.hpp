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

#ifndef PACKET_HPP
#define PACKET_HPP

#include <vector>
#include <cstdint>
#include <chrono>

class Packet {
	public:
		Packet(const std::vector<uint8_t> &bytes);
		Packet(const uint8_t *data, size_t size);

		void setBytes(const std::vector<uint8_t> &bytes);
		const std::vector<uint8_t>& getBytes() const;

		const std::chrono::high_resolution_clock::time_point& getCreationTimePoint() const;

	private:
		std::vector<uint8_t> bytes;
		std::chrono::high_resolution_clock::time_point creation_time_point = std::chrono::high_resolution_clock::now();
};

#endif
