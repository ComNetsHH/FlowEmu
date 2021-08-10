#######################################
#                                     #
#               BUILDER               #
#                                     #
#######################################
FROM ubuntu:20.04 AS builder

# Setup APT
ENV DEBIAN_FRONTEND noninteractive

# Install build dependencies
RUN apt-get update \
	&& apt-get install -y build-essential pkg-config cmake ccache libboost-dev libboost-program-options-dev libmosquitto-dev libjsoncpp-dev \
	&& rm -rf /var/lib/apt/lists/*


#######################################
#                                     #
#                BUILD                #
#                                     #
#######################################
FROM builder AS build

# Add FlowEmu source
ADD CMakeLists.txt /flowemu/CMakeLists.txt
ADD src /flowemu/src

# Build FlowEmu
RUN mkdir -p /flowemu/BUILD \
	&& cd /flowemu/BUILD \
	&& cmake .. \
	&& make -j$(nproc)


#######################################
#                                     #
#               RUNTIME               #
#                                     #
#######################################
FROM ubuntu:20.04

# Setup APT
ENV DEBIAN_FRONTEND noninteractive

# Install runtime dependencies
RUN apt-get update \
	&& apt-get install -y libboost-program-options1.71.0 libmosquitto1 libjsoncpp1 iproute2 iputils-ping iperf \
	&& rm -rf /var/lib/apt/lists/*

# Install FlowEmu
COPY --from=build /flowemu/BUILD/bin/* /usr/bin/
