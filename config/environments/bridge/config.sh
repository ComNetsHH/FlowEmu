#!/bin/bash

# Network interface names
INTERFACE_SOURCE="emulator0"
INTERFACE_SINK="emulator1"

# Build normal FlowEmu Docker image
DOCKER_FILE_FLOWEMU="Dockerfile"
DOCKER_IMAGE_FLOWEMU="flowemu"

# Build FlowEmu Docker image with machine learning support
#DOCKER_FILE_FLOWEMU="Dockerfile_ml"
#DOCKER_IMAGE_FLOWEMU="flowemu_ml"
