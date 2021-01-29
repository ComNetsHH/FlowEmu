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
