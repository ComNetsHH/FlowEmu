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
	&& apt-get install -y build-essential cmake ccache libboost-dev libmosquitto-dev libjsoncpp-dev  \
	&& rm -rf /var/lib/apt/lists/*


#######################################
#                                     #
#                BUILD                #
#                                     #
#######################################
FROM builder

# Build channel emulator
ADD . /channel_emulator
RUN rm -R /channel_emulator/BUILD; mkdir -p /channel_emulator/BUILD \
	&& cd /channel_emulator/BUILD \
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
	&& apt-get install -y libmosquitto1 libjsoncpp1 iproute2 iputils-ping \
	&& rm -rf /var/lib/apt/lists/*

# Install channel emulator
COPY --from=build /channel_emulator/BUILD/bin/* /usr/bin/
