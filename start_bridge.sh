#!/bin/bash

# Network interface names
INTERFACE_SOURCE="emulator0"
INTERFACE_SINK="emulator1"

# MQTT broker
MQTT_BROKER="$HOSTNAME"

# Build normal emulator Docker image
DOCKER_FILE_EMULATOR="Dockerfile"
DOCKER_IMAGE_EMULATOR="channel_emulator"

# Build emulator Docker image with machine learning support
#DOCKER_FILE_EMULATOR="Dockerfile_ml"
#DOCKER_IMAGE_EMULATOR="channel_emulator_ml"

cleanup() {
	sudo ip netns exec channel ip link set $INTERFACE_SOURCE netns 1 || true
	sudo ip netns exec channel ip link set $INTERFACE_SINK netns 1 || true

	(docker stop channel; docker rm channel) || true

	sudo ip netns delete channel || true
}

setup_network() {
	pid_channel=$(docker inspect -f '{{.State.Pid}}' channel)
	sudo mkdir -p /var/run/netns/
	sudo ln -sfT /proc/$pid_channel/ns/net /var/run/netns/channel

	sudo ip link set $INTERFACE_SOURCE promisc on
	sudo ethtool -K $INTERFACE_SOURCE tcp-segmentation-offload off generic-segmentation-offload off generic-receive-offload off
	sudo ip link set $INTERFACE_SOURCE netns channel
	sudo ip netns exec channel ip link set $INTERFACE_SOURCE up

	sudo ip link set $INTERFACE_SINK promisc on
	sudo ethtool -K $INTERFACE_SINK tcp-segmentation-offload off generic-segmentation-offload off generic-receive-offload off
	sudo ip link set $INTERFACE_SINK netns channel
	sudo ip netns exec channel ip link set $INTERFACE_SINK up
}

# Exit on error
set -o errexit

# Stop and delete old Docker container and network namespace
cleanup &> /dev/null

# Build Docker image
docker build -t $DOCKER_IMAGE_EMULATOR -f $DOCKER_FILE_EMULATOR .

# Start Docker container
docker run -d -it -v $(pwd)/config:/config --name channel $DOCKER_IMAGE_EMULATOR

# Add interfaces to Docker container
setup_network > /dev/null

# Start emulator
docker exec -it channel channel_emulator --mqtt-host=$MQTT_BROKER --interface-source=$INTERFACE_SOURCE --interface-sink=$INTERFACE_SINK

# Stop and delete Docker container and network namespace
cleanup > /dev/null
