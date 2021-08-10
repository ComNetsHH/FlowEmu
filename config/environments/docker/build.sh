#!/bin/bash

source config/environments/docker/config.sh

# Build FlowEmu Docker image
docker build -t $DOCKER_IMAGE_FLOWEMU -f $DOCKER_FILE_FLOWEMU .
