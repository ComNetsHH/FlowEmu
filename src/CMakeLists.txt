find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP jsoncpp)

if(JSONCPP_INCLUDE_DIRS)
	include_directories("${JSONCPP_INCLUDE_DIRS}")
endif(JSONCPP_INCLUDE_DIRS)

set(channel_emulator_SRCS
	main.cpp
	modules/ModuleManager.cpp
	modules/delay/FixedDelayModule.cpp
	modules/loss/GilbertElliotLossModule.cpp
	modules/loss/UncorrelatedLossModule.cpp
	modules/meter/DelayMeter.cpp
	modules/meter/ThroughputMeter.cpp
	modules/null/NullModule.cpp
	modules/queue/FifoQueueModule.cpp
	modules/rate/BitrateRateModule.cpp
	modules/rate/FixedIntervalRateModule.cpp
	modules/rate/TraceRateModule.cpp
	modules/socket/RawSocket.cpp
	utils/Mqtt.cpp
	utils/Packet.cpp
)

add_executable(channel_emulator ${channel_emulator_SRCS})

set_property(TARGET channel_emulator PROPERTY CXX_STANDARD 11)
set_property(TARGET channel_emulator PROPERTY CXX_STANDARD_REQUIRED ON)

target_link_libraries(channel_emulator
	mosquitto
	${JSONCPP_LIBRARIES}
	pthread
)

if(MACHINE_LEARNING)
	find_package(TensorflowCC REQUIRED)
	find_package(Protobuf REQUIRED)

	if(Protobuf_INCLUDE_DIRS)
		include_directories("${Protobuf_INCLUDE_DIRS}")
	endif(Protobuf_INCLUDE_DIRS)

	target_compile_definitions(channel_emulator PRIVATE MACHINE_LEARNING)

	target_sources(channel_emulator PRIVATE
		ml/DeepQLearning.cpp
		modules/queue/DQLQueueModule.cpp
	)

	target_link_libraries(channel_emulator
		TensorflowCC::TensorflowCC
		${Protobuf_LIBRARIES}
	)
endif(MACHINE_LEARNING)

install(TARGETS channel_emulator RUNTIME DESTINATION bin)
