cmake_minimum_required(VERSION 3.0)

project(fmt)

set(SOURCES
	src/format.cc
)

include_directories(src)

add_library(fmt SHARED ${SOURCES})

