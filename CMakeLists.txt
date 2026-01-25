cmake_minimum_required(VERSION 4.1)
project(csc2525)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(csc2525
        lz77.cpp
        lz77.h
        main.cpp)
