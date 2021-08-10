#!/bin/bash

# Build normal FlowEmu Docker image
DOCKER_FILE_FLOWEMU="Dockerfile"
DOCKER_IMAGE_FLOWEMU="flowemu"

# Build FlowEmu Docker image with machine learning support
#DOCKER_FILE_FLOWEMU="Dockerfile_ml"
#DOCKER_IMAGE_FLOWEMU="flowemu_ml"

# Use FlowEmu Docker image for source and sink
DOCKER_IMAGE_SOURCE_SINK=$DOCKER_IMAGE_FLOWEMU
