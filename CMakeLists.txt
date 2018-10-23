cmake_minimum_required(VERSION 3.0.0)
project(dramsim3)

# for config file parsing
add_library(ini SHARED
    ext/inih/src/INIReader.cpp
    ext/inih/src/ini.c
)
target_compile_options(ini PRIVATE -O3)
target_include_directories(ini PUBLIC ext/inih/src)

# for printing
add_library(format SHARED
    ext/fmt/src/format.cc
)
target_compile_options(format PRIVATE -O3)
target_include_directories(format INTERFACE ext/fmt/src)
# had to do this the old way to be able to use lower cmake versions
set_target_properties(format PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

# argparsing library, only used in main program not the library
add_library(args INTERFACE)
target_include_directories(args INTERFACE ext/headers)

# Main DRAMSim Lib
add_library(dramsim3 SHARED
    src/bankstate.cc
    src/channel_state.cc
    src/command_queue.cc
    src/common.cc
    src/configuration.cc
    src/controller.cc
    src/dram_system.cc
    src/hmc.cc
    src/refresh.cc
    src/statistics.cc
    src/timing.cc
    src/memory_system.cc
)

if (THERMAL)
    # dependency check
    # sudo apt-get install libatlas-base-dev on ubuntu
    find_package(BLAS REQUIRED)
    find_package(OpenMP REQUIRED)
    # YOU need to build superlu on your own. Do the following:
    # git submodule update --init
    # cd ext/SuperLU_MT_3.1 && make lib
    find_library(SUPERLU superlu_mt_OPENMP PATH_SUFFIXES ext/SuperLU_MT_3.1/lib/)

    target_link_libraries(dramsim3
        PRIVATE ${SUPERLU} f77blas atlas m ${OpenMP_C_FLAGS}
    )
    target_sources(dramsim3
        PRIVATE src/thermal.cc src/sp_ienv.c src/thermal_solver.c
    )
    target_compile_options(dramsim3 PRIVATE -DTHERMAL -D_LONGINT -DAdd_ ${OpenMP_C_FLAGS})

    add_executable(thermalreplay src/thermal_replay.cc)
    target_link_libraries(thermalreplay dramsim3)
    target_compile_options(thermalreplay PRIVATE -DTHERMAL -D_LONGINT -DAdd_ ${OpenMP_C_FLAGS})
endif (THERMAL)

target_include_directories(dramsim3 INTERFACE src)
target_compile_options(dramsim3 PRIVATE -O3)
target_link_libraries(dramsim3 PRIVATE ini format)
set_target_properties(dramsim3 PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

# trace CPU, .etc
add_executable(dramsim3main src/main.cc src/cpu.cc)
target_link_libraries(dramsim3main PRIVATE dramsim3 args)
target_compile_options(dramsim3main PRIVATE -O3)
set_target_properties(dramsim3main PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)
