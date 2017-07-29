cmake_minimum_required(VERSION 3.0)

project(inih)

set(SOURCES
	src/ini.c
	src/INIReader.cpp
)

include_directories(src)

add_library(inih SHARED ${SOURCES})

