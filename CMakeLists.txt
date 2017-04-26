cmake_minimum_required(VERSION 3.0)

project(dramcore)

add_subdirectory("ext/inih")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-Wall -fPIC")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -DDEBUG_OUTPUT")
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3")

set(SOURCES
        src/bankstate.cc
        src/channel_state.cc
        src/command_queue.cc
        src/common.cc
        src/configuration.cc
        src/controller.cc
        src/cpu.cc
        src/refresh.cc
        src/statistics.cc
        src/timing.cc
        src/memory_system.cc
        )

set(SOURCES_TEST
        test/main.cc
        test/test.cc
        )

include_directories(src ext/headers)

add_library(dramcore SHARED ${SOURCES})
target_link_libraries(dramcore inih)
set_target_properties(dramcore PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(dramcoremain src/main.cc)
target_link_libraries(dramcoremain dramcore)
#set_target_properties(dramcore PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(dramcoretest EXCLUDE_FROM_ALL ${SOURCES_TEST})
target_link_libraries(dramcoretest dramcore)
#set_target_properties(dramcoretest PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release")
        message(STATUS "CMAKE_BUILD_TYPE not specified, using RELEASE mode by default")
        message(STATUS "To change it, using cmake -DCMAKE_BUILD_TYPE=DEBUG/RELEASE")
endif(NOT CMAKE_BUILD_TYPE)


message("CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")
# set_target_properties(dramcore PROPERTIES DEBUG_POSTFIX "_d")


# add validation flow here to make life easier...
add_executable(validation EXCLUDE_FROM_ALL src/main.cc ${SOURCES})
target_link_libraries(validation inih)
set_target_properties(validation PROPERTIES OUTPUT_NAME dramcore-validate)  # reuse same output name
# target_compile_definitions(validation PRIVATE -DVALIDATION)
add_custom_command(TARGET validation
                   POST_BUILD
                   COMMAND ./dramcore-validate -c configs/ddr4_8G_x8_config.ini -n 10000
                   # post processing scripts here...
                   COMMAND rm dramcore-validate
)

