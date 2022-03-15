#!/bin/bash

# Build normal FlowEmu Docker image
DOCKER_FILE_FLOWEMU="Dockerfile_debug"
DOCKER_IMAGE_FLOWEMU="flowemu_debug"

# Build FlowEmu Docker image with machine learning support
#DOCKER_FILE_FLOWEMU="Dockerfile_ml_debug"
#DOCKER_IMAGE_FLOWEMU="flowemu_ml_debug"

# Use FlowEmu Docker image for source and sink
DOCKER_IMAGE_SOURCE_SINK=$DOCKER_IMAGE_FLOWEMU
