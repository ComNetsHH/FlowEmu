#!/bin/bash

source config/environments/bridge/config.sh

# Start Docker container
docker run -d -it -v $(pwd)/config:/config --name channel $DOCKER_IMAGE_FLOWEMU

# Add interfaces to Docker container
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

setup_network > /dev/null
