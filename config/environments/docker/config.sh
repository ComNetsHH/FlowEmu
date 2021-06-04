#!/bin/sh

# Build normal emulator Docker image
DOCKER_FILE_EMULATOR="Dockerfile"
DOCKER_IMAGE_EMULATOR="channel_emulator"

# Build emulator Docker image with machine learning support
#DOCKER_FILE_EMULATOR="Dockerfile_ml"
#DOCKER_IMAGE_EMULATOR="channel_emulator_ml"

# User emulator Docker image for source and sink
DOCKER_IMAGE_SOURCE_SINK=$DOCKER_IMAGE_EMULATOR
