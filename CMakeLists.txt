cmake_minimum_required(VERSION 3.0)

project(dramcore)

add_subdirectory("ext/inih")

set(CMAKE_CXX_STANDARD 11)
# set(CMAKE_CXX_FLAGS "-O0")
# set(CMAKE_CXX_FLAGS "-O0 -Wall")
set(CMAKE_CXX_FLAGS "-fPIC")

# cmake -DCMAKE_BUILD_TYPE=Debug
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -Wall")

# cmake -DCMAKE_BUILD_TYPE=Release
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O2 -Wall")



if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release")
        message(STATUS "CMAKE_BUILD_TYPE not specified, using RELEASE mode by default")
        message(STATUS "To change it, using cmake -DCMAKE_BUILD_TYPE=DEBUG/RELEASE")
endif(NOT CMAKE_BUILD_TYPE)

message(STATUS "CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")

# if (CMAKE_BUILD_TYPE MATCHES Debug)
message("CMAKE flags will be: ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG}")


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

set(SOURCES_TEST test/main.cc
                 test/test.cc
)

include_directories(src ext/src)

add_library(dramcore SHARED ${SOURCES})
target_link_libraries(dramcore inih)
set_target_properties(dramcore PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})


add_executable(dramcoremain src/main.cc)
target_link_libraries(dramcoremain dramcore)
#set_target_properties(dramcore PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# add test target, not default
add_executable(test EXCLUDE_FROM_ALL ${SOURCES_TEST})
target_link_libraries(dramcoretest dramcore)
set_target_properties(test PROPERTIES OUTPUT_NAME dramcoretest)
#set_target_properties(dramcoretest PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})


# add debug target
add_executable(debug EXCLUDE_FROM_ALL src/main.cc)
target_link_libraries(debug dramcore)
set_target_properties(debug PROPERTIES OUTPUT_NAME dramcore-debug)
target_compile_definitions(debug PRIVATE DEBUG=1)
# add_custom_command(TARGET debug POST_BUILD COMMAND ls)

