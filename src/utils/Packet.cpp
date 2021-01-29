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