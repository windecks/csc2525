cmake_minimum_required(VERSION 3.5)
project(csc2525)

set(CMAKE_CXX_STANDARD 17)

include_directories(.)

add_executable(csc2525
        lz77.cpp
        lz77.h
        main.cpp
        bit_writer.h
        constants.h)
