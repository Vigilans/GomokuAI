cmake_minimum_required (VERSION 3.6)

project(CoreInterface)

add_subdirectory(../lib)
include_directories(./)

add_executable(CoreInterface,
    src/main.cpp
)
