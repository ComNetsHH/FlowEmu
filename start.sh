#!/bin/bash

# Build normal emulator Docker image
DOCKER_FILE_EMULATOR="Dockerfile"
DOCKER_IMAGE_EMULATOR="channel_emulator"

# Build emulator Docker image with machine learning support
#DOCKER_FILE_EMULATOR="Dockerfile_ml"
#DOCKER_IMAGE_EMULATOR="channel_emulator_ml"

# User emulator Docker image for source and sink
DOCKER_IMAGE_SOURCE_SINK=$DOCKER_IMAGE_EMULATOR

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
docker build -t $DOCKER_IMAGE_EMULATOR -f $DOCKER_FILE_EMULATOR .

# Start Docker containers
# NOTE: The source and sink containers are started with extended privileges in order to be able to use all congestion control modules of the host system in iPerf.
docker run -d -it -v $(pwd)/config:/config --name channel $DOCKER_IMAGE_EMULATOR
docker run -d -it --privileged --name source $DOCKER_IMAGE_SOURCE_SINK
docker run -d -it --privileged --name sink $DOCKER_IMAGE_SOURCE_SINK

# Create links between Docker containers
setup_network > /dev/null

# Start emulator
docker exec -it channel channel_emulator

# Stop and delete Docker containers and network namespaces
cleanup > /dev/null
