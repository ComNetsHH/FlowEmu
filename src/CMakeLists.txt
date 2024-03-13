find_package(PkgConfig REQUIRED)

find_package(Boost COMPONENTS program_options REQUIRED)
pkg_check_modules(JSONCPP jsoncpp)

include_directories("${Boost_INCLUDE_DIR}")

if(JSONCPP_INCLUDE_DIRS)
	include_directories("${JSONCPP_INCLUDE_DIRS}")
endif(JSONCPP_INCLUDE_DIRS)

set(flowemu_SRCS
	main.cpp
	modules/ModuleManager.cpp
	modules/delay/FixedDelayModule.cpp
	modules/delay/TraceDelayModule.cpp
	modules/loss/GilbertElliotLossModule.cpp
	modules/loss/TraceLossModule.cpp
	modules/loss/UncorrelatedLossModule.cpp
	modules/meter/DelayMeter.cpp
	modules/meter/ThroughputMeter.cpp
	modules/null/NullModule.cpp
	modules/queue/CodelQueueModule.cpp
	modules/queue/FifoQueueModule.cpp
	modules/queue/PieQueueModule.cpp
	modules/queue/Pi2QueueModule.cpp
	modules/queue/RedQueueModule.cpp
	modules/rate/BitrateRateModule.cpp
	modules/rate/FixedIntervalRateModule.cpp
	modules/rate/TraceRateModule.cpp
	modules/socket/RawSocket.cpp
	utils/Mqtt.cpp
	utils/Packet.cpp
)

add_executable(flowemu ${flowemu_SRCS})

set_property(TARGET flowemu PROPERTY CXX_STANDARD 17)
set_property(TARGET flowemu PROPERTY CXX_STANDARD_REQUIRED ON)

target_link_libraries(flowemu
	${Boost_LIBRARIES}
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

	target_compile_definitions(flowemu PRIVATE MACHINE_LEARNING)

	target_sources(flowemu PRIVATE
		ml/DeepQLearning.cpp
		modules/queue/DQLQueueModule.cpp
	)

	target_link_libraries(flowemu
		TensorflowCC::TensorflowCC
		${Protobuf_LIBRARIES}
	)
endif(MACHINE_LEARNING)

install(TARGETS flowemu RUNTIME DESTINATION bin)
