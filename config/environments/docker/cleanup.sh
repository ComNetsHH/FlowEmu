#!/bin/sh

# Stop and delete Docker containers and network namespaces
cleanup() {
	(docker stop channel; docker rm channel) || true
	(docker stop source; docker rm source) || true
	(docker stop sink; docker rm sink) || true

	sudo ip netns delete channel || true
	sudo ip netns delete source || true
	sudo ip netns delete sink || true
}

cleanup &> /dev/null
