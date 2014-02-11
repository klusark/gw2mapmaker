cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(gw2mapmaker)

set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAG}")


set(SOURCES
src/main.cpp
src/dxt.cpp
)

set(HEADERS
src/main.h
src/dxt.h
)

add_executable(gw2mapmaker ${SOURCES} ${HEADERS})

