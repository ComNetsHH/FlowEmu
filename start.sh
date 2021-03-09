#!/bin/bash

cleanup() {
	(docker stop channel; docker rm channel) || true
	(docker stop source; docker rm source) || true
	(docker stop sink; docker rm sink) || true

	sudo ip netns delete channel || true
	sudo ip netns delete source || true
	sudo ip netns delete sink || true
}

setup_network() {
	pid_channel=$(docker inspect -f '{{.State.Pid}}' channel)
	pid_source=$(docker inspect -f '{{.State.Pid}}' source)
	pid_sink=$(docker inspect -f '{{.State.Pid}}' sink)

	sudo mkdir -p /var/run/netns/
	sudo ln -sfT /proc/$pid_channel/ns/net /var/run/netns/channel
	sudo ln -sfT /proc/$pid_source/ns/net /var/run/netns/source
	sudo ln -sfT /proc/$pid_sink/ns/net /var/run/netns/sink

	sudo ip link add emulator netns source type veth peer name in netns channel
	sudo ip netns exec source ethtool -K emulator rx off tx off
	sudo ip netns exec source ip addr add 10.0.1.1/24 dev emulator
	sudo ip netns exec channel ip link set in up
	sudo ip netns exec source ip link set lo up
	sudo ip netns exec source ip link set emulator up
	sudo ip netns exec source ip route add 10.0.2.0/24 dev emulator

	sudo ip link add emulator netns sink type veth peer name out netns channel
	sudo ip netns exec sink ethtool -K emulator rx off tx off
	sudo ip netns exec sink ip addr add 10.0.2.1/24 dev emulator
	sudo ip netns exec channel ip link set out up
	sudo ip netns exec sink ip link set lo up
	sudo ip netns exec sink ip link set emulator up
	sudo ip netns exec sink ip route add 10.0.1.0/24 dev emulator
}

# Exit on error
set -o errexit

# Stop and delete old Docker containers and network namespaces
cleanup &> /dev/null

# Build Docker image
docker build -t channel_emulator .

# Start Docker containers
docker run -d -it -v $(pwd)/config:/config --name channel channel_emulator
docker run -d -it --privileged --name source channel_emulator
docker run -d -it --privileged --name sink channel_emulator

# Create links between Docker containers
setup_network > /dev/null

# Start emulator
docker exec -it channel channel_emulator

# Stop and delete Docker containers and network namespaces
cleanup > /dev/null
