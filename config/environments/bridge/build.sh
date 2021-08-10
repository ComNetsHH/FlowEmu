#!/bin/bash

source config/environments/bridge/config.sh

# Build FlowEmu Docker image
docker build -t $DOCKER_IMAGE_FLOWEMU -f $DOCKER_FILE_FLOWEMU .
