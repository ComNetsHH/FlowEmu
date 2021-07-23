#!/bin/bash

source config/environments/bridge/config.sh

# Stop and delete Docker container and network namespace
cleanup() {
	sudo ip netns exec channel ip link set $INTERFACE_SOURCE netns 1 || true
	sudo ip netns exec channel ip link set $INTERFACE_SINK netns 1 || true

	(docker stop channel; docker rm channel) || true

	sudo ip netns delete channel || true
}

cleanup &> /dev/null
