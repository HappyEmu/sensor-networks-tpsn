cmake_minimum_required(VERSION 3.3)
project(sensor_networks_tpsn)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(../../core/ ../../platform/sky ../../core/sys)

add_definitions(-TARGET=sky)

set(SOURCE_FILES
    tpsn.c
    tpsn.h)

add_executable(sensor_networks_tpsn ${SOURCE_FILES})