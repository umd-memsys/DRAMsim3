cmake_minimum_required(VERSION 3.0.0)
project(dramsim3)
add_library(ini SHARED
    ext/inih/src/INIReader.cpp
    ext/inih/src/ini.c
)
target_compile_options(ini PRIVATE -O3)
target_include_directories(ini PUBLIC ext/inih/src)
add_library(format SHARED
    ext/fmt/src/format.cc
)
target_compile_options(format PRIVATE -O3)

target_include_directories(format INTERFACE ext/fmt/src)
# argparsing library, only used in main program not the library
add_library(args INTERFACE)
target_include_directories(args INTERFACE ext/headers)
add_library(dramsim3 SHARED
    src/bankstate.cc
    src/channel_state.cc
    src/command_queue.cc
    src/common.cc
    src/configuration.cc
    src/controller.cc
    src/cpu.cc
    src/dram_system.cc
    src/hmc.cc
    src/refresh.cc
    src/statistics.cc
    src/timing.cc
    src/memory_system.cc
)
target_include_directories(dramsim3 INTERFACE src)
target_compile_options(dramsim3 PRIVATE -DNDEBUG -O3)
target_link_libraries(dramsim3
    PRIVATE
        ini
        format
)
add_executable(dramsim3main src/main.cc)
target_link_libraries(dramsim3main PRIVATE dramsim3 args)
target_compile_options(dramsim3main PRIVATE -O3)