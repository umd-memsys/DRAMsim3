cmake_minimum_required(VERSION 3.0)

project(dram-core)

set(CMAKE_CXX_STANDARD 11)

file(GLOB SOURCES "src/*.cc")

set(CMAKE_CXX_FLAGS "-O0 -Wall")
message("CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")

add_executable(dramcore ${SOURCES})

# set_target_properties(dramcore PROPERTIES DEBUG_POSTFIX "_d")
set_target_properties(dramcore PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})