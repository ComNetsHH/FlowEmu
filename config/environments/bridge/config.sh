#!/bin/bash

# Network interface names
INTERFACE_SOURCE="emulator0"
INTERFACE_SINK="emulator1"

# Build normal emulator Docker image
DOCKER_FILE_EMULATOR="Dockerfile"
DOCKER_IMAGE_EMULATOR="channel_emulator"

# Build emulator Docker image with machine learning support
#DOCKER_FILE_EMULATOR="Dockerfile_ml"
#DOCKER_IMAGE_EMULATOR="channel_emulator_ml"
