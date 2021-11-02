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

size_t Packet::parseEthernetHeader(uint16_t &type_field) {
	size_t type_field_offset = 12;
	size_t network_layer_offset = 14;

	// Parse type field
	type_field = (bytes[type_field_offset] << 8) | bytes[type_field_offset+1];

	// Handle 802.1Q header
	if(type_field == 0x8100) {
		type_field_offset += 4;
		network_layer_offset += 4;

		type_field = (bytes[type_field_offset] << 8) | bytes[type_field_offset+1];
	}

	return network_layer_offset;
}

void Packet::updateIPv4HeaderChecksum() {
	uint16_t type_field;
	size_t network_layer_offset = parseEthernetHeader(type_field);

	// Handle type field
	if(type_field != 0x0800) {
		// Not IPv4
		return;
	}

	uint32_t checksum = 0;

	// Get IP header length
	size_t ip_header_length = (bytes[network_layer_offset] & 0x0F) * 4;

	// Sum up header fields
	for(size_t i = 0; i < ip_header_length; i += 2) {
		if(i == 10) {
			continue;
		}

		checksum += (bytes[network_layer_offset+i] << 8) | bytes[network_layer_offset+i+1];
	}

	// Add end-around carry
	while(checksum & 0xFFFF0000) {
		checksum = (checksum & 0xFFFF) + (checksum >> 16);
	}

	// Calculate ones' complement
	checksum = ~checksum;

	// Write checksum to packet
	bytes[network_layer_offset+10] = (checksum >> 8);
	bytes[network_layer_offset+11] = checksum;
}

uint8_t Packet::getECN() {
	uint16_t type_field;
	size_t network_layer_offset = parseEthernetHeader(type_field);

	// Handle type field
	if(type_field == 0x0800) {
		// IPv4
		return bytes[network_layer_offset+1] & 0b00000011;
	} else if(type_field == 0x86DD) {
		// IPv6
		return ((bytes[network_layer_offset+1] & 0b00110000) >> 4);
	}

	return 0;
}

void Packet::setECN(uint8_t ecn) {
	uint16_t type_field;
	size_t network_layer_offset = parseEthernetHeader(type_field);

	// Handle type field
	if(type_field == 0x0800) {
		// IPv4
		bytes[network_layer_offset+1] = (bytes[network_layer_offset+1] & ~0b00000011) | (ecn & 0b00000011);
	} else if(type_field == 0x86DD) {
		// IPv6
		bytes[network_layer_offset+1] = (bytes[network_layer_offset+1] & ~0b00110000) | ((ecn << 4) & 0b00110000);
	}
}
