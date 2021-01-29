#find_package(Jsoncpp REQUIRED)

#include_directories(
#	${JSONCPP_INCLUDE_DIRS}/${JSONCPP_INCLUDE_PREFIX}
#)

set(channel_emulator_SRCS
	main.cpp
	modules/ModuleManager.cpp
	modules/delay/FixedDelayModule.cpp
	modules/loss/UncorrelatedLossModule.cpp
	modules/meter/DelayMeter.cpp
	modules/meter/ThroughputMeter.cpp
	modules/null/NullModule.cpp
	modules/rate/FixedIntervalRateModule.cpp
	modules/socket/RawSocket.cpp
	utils/Mqtt.cpp
	utils/Packet.cpp
)

add_executable(channel_emulator ${channel_emulator_SRCS})

set_property(TARGET channel_emulator PROPERTY CXX_STANDARD 11)
set_property(TARGET channel_emulator PROPERTY CXX_STANDARD_REQUIRED ON)

target_link_libraries(channel_emulator
	mosquitto
	jsoncpp
	pthread
)

install(TARGETS channel_emulator RUNTIME DESTINATION bin)
