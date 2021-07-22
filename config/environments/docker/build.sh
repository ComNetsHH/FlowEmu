#!/bin/bash

source config/environments/docker/config.sh

# Build Docker image
docker build -t $DOCKER_IMAGE_EMULATOR -f $DOCKER_FILE_EMULATOR .
