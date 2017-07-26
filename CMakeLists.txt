cmake_minimum_required(VERSION 3.0)

project(dramcore)

add_subdirectory(ext/inih)
add_subdirectory(ext/fmt)

# self-defined flgas that help debugging
# use cmake -DGENERATE_TRACE=ON or 1 to enbale this
if (GENERATE_TRACE)
        message("Adding GENERATE_TRACE definition...")
        add_definitions(-DGENERATE_TRACE)
endif (GENERATE_TRACE)

if (DEBUG_POWER)
        message("Adding DEBUG_POWER definition...")
        add_definitions(-DDEBUG_POWER)
endif  (DEBUG_POWER)

if (DEBUG_HMC)
        message("Adding DEBUG_HMC definition...")
        add_definitions(-DDEBUG_HMC)
endif (DEBUG_HMC)

if (THERMAL)
        message("Adding thermal module...")
        add_definitions(-DTHERMAL)

        # since SuperLU has a build system already, I don't want to 
        # rewirte the entire make system so here's the solution...
        if (NOT SUPERLU_SRC)
                set(SUPERLU_SRC ${CMAKE_SOURCE_DIR}/ext/SuperLU_MT_3.1/)
        endif (NOT SUPERLU_SRC)
        set(SUPERLU_PLAT PTHREAD)
        set(SUPERLU_LIB ${SUPERLU_SRC}/lib/libsuperlu_mt_${SUPERLU_PLAT}.a)
        add_library(superlu_mt STATIC IMPORTED)
        set_target_properties(superlu_mt 
                              PROPERTIES IMPORTED_LOCATION ${SUPERLU_LIB})
        add_custom_target(build_superlu_mt
                          COMMAND make lib 
                          WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/ext/SuperLU_MT_3.1/
        )
        add_dependencies(superlu_mt build_superlu_mt)
        include_directories(${SUPERLU_SRC}/SRC)  # header files are here..
        
endif (THERMAL)

set(EXTRA_CXX_FLAGS "-Wall -fPIC")
set(EXTRA_CXX_FLAGS_DEBUG "-O0 -DDEBUG_OUTPUT")
set(EXTRA_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(EXTRA_CXX_FLAGS_RELWITHDEBINFO "-O0 -DNDDEBUG")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${EXTRA_CXX_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${EXTRA_CXX_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO  "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${EXTRA_CXX_FLAGS_RELWITHDEBINFO}")

set(SOURCES
        src/bankstate.cc
        src/channel_state.cc
        src/command_queue.cc
        src/common.cc
        src/configuration.cc
        src/controller.cc
        src/cpu.cc
        src/hmc.cc
        src/refresh.cc
        src/statistics.cc
        src/timing.cc
        src/memory_system.cc
        )

if (THERMAL)
        set(SOURCES ${SOURCES} src/thermal.cc)
endif (THERMAL)

set(SOURCES_TEST
        test/main.cc
        test/test.cc
        )

include_directories(src ext/headers)



add_library(dramcore SHARED ${SOURCES})

if (THERMAL)
        target_link_libraries(dramcore inih fmt superlu_mt)
else (THERMAL)
        target_link_libraries(dramcore inih fmt)
endif (THERMAL)

set_target_properties(dramcore PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(dramcoremain src/main.cc)
target_link_libraries(dramcoremain dramcore)
#set_target_properties(dramcore PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(dramcoretest ${SOURCES_TEST})
target_link_libraries(dramcoretest dramcore)
#set_target_properties(dramcoretest PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

message("CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")
# set_target_properties(dramcore PROPERTIES DEBUG_POSTFIX "_d")

