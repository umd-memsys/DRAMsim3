cmake_minimum_required(VERSION 3.0)

project(dramcore)

add_subdirectory(ext/inih)
add_subdirectory(ext/fmt)

# self-defined flgas that help debugging
# use cmake -DGENERATE_TRACE=ON or 1 to enbale this
if (NO_BACKPRESSURE)
        message("Adding NO_BACKPRESSURE - To enable infinite buffering of memory requests in memory controller to support CPUs that does not model backpressure when the queues in the memory controller are full")
        add_definitions(-DNO_BACKPRESSURE)
endif (NO_BACKPRESSURE)

if (GENERATE_TRACE)
        message("Adding GENERATE_TRACE - To log to a file the memory address trace along with timing information")
        add_definitions(-DGENERATE_TRACE)
endif (GENERATE_TRACE)

if (DEBUG_POWER)
        message("Adding DEBUG_POWER definition...")
        add_definitions(-DDEBUG_POWER)
endif  (DEBUG_POWER)


if (DEBUG_LOC_MAPPING)
        add_definitions(-DDEBUG_LOC_MAPPING)
endif (DEBUG_LOC_MAPPING)

if (THERMAL)
        message("Adding thermal module...")
        add_definitions(-DTHERMAL)

        # check dependency
        find_package(Threads REQUIRED)
        find_package(BLAS REQUIRED)
        find_package(OpenMP REQUIRED)

        # since SuperLU has a build system already, I don't want to 
        # rewirte the entire make system so here's the solution...
        if (NOT SUPERLU_ROOT)
                set(SUPERLU_ROOT ${CMAKE_SOURCE_DIR}/ext/SuperLU_MT_3.1/)
                execute_process(COMMAND git submodule update --init -- ${SUPERLU_ROOT})
        endif (NOT SUPERLU_ROOT)
        set(SUPERLU_PLAT OPENMP)
        set(SUPERLU_LIB ${SUPERLU_ROOT}/lib/libsuperlu_mt_${SUPERLU_PLAT}.a)
        add_library(superlu_mt SHARED IMPORTED)
        
        # add_library(superlu_blas STATIC IMPORTED)
        set_target_properties(superlu_mt 
                              PROPERTIES IMPORTED_LOCATION ${SUPERLU_LIB})
        add_custom_target(build_superlu_mt
                          COMMAND make lib 
                          WORKING_DIRECTORY ${SUPERLU_ROOT}
        )
        add_dependencies(superlu_mt build_superlu_mt)
        include_directories(${SUPERLU_ROOT}/SRC)  # header files are here..
        
endif (THERMAL)

set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-fPIC -D_LONGINT -DAdd_")
set(EXTRA_CXX_FLAGS "-Wall -fPIC")
set(EXTRA_CXX_FLAGS_DEBUG "-O0 -DDEBUG_OUTPUT")
set(EXTRA_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(EXTRA_CXX_FLAGS_RELWITHDEBINFO "-O0 -DNDDEBUG")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)
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
        set(SOURCES ${SOURCES} src/thermal.cc src/sp_ienv.c src/thermal_solver.c)
        if (OPENMP_FOUND)
            message("adding openmp flag")
            set_source_files_properties(src/thermal_solver.c PROPERTIES COMPILE_FLAGS "${OpenMP_C_FLAGS} -O2")
        endif(OPENMP_FOUND)
endif (THERMAL)

set(SOURCES_TEST
        test/main.cc
        test/test.cc
        )

include_directories(src ext/headers)



add_library(dramcore SHARED ${SOURCES})

if (THERMAL)
        target_link_libraries(dramcore inih fmt superlu_mt ${CMAKE_THREAD_LIBS_INIT} m f77blas atlas ${OpenMP_C_FLAGS})
        add_executable(thermalreplay src/thermal_replay.cc)
        target_link_libraries(thermalreplay dramcore)
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

