project(channel_emulator)
cmake_minimum_required(VERSION 3.5)

option(MACHINE_LEARNING "Build with machine learning support" OFF)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

add_subdirectory(src)
