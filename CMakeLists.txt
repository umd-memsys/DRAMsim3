cmake_minimum_required(VERSION 3.0)

project(dram-core)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-O0 -Wall")

add_subdirectory(googletest)

# file(GLOB SOURCES "src/*.cc")
# file(GLOB SOURCES_TEST "test/*.cc")

set(SOURCES src/bankstate.cc
            src/channel_state.cc
            src/command_queue.cc
            src/common.cc
            src/config.cc
            src/controller.cc
            src/cpu.cc
            src/refresh.cc
            src/timing.cc
)

set(SOURCES_TEST test/timing_test.cc
)

include_directories(src)

add_library(dramcorelib SHARED ${SOURCES})

add_executable(dramcore src/main.cc)
target_link_libraries(dramcore dramcorelib)
#set_target_properties(dramcore PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(dramcoretest ${SOURCES_TEST})
target_link_libraries(dramcoretest gtest_main dramcorelib)
#set_target_properties(dramcoretest PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

message("CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")
# set_target_properties(dramcore PROPERTIES DEBUG_POSTFIX "_d")
