cmake_minimum_required(VERSION 3.8)

project(CorePyExt)

find_package(pybind11 CONFIG REQUIRED)
include_directories(./)

add_library(CorePyExt MODULE pch.cpp src/module.cpp)
target_link_libraries(CorePyExt PRIVATE 
    CoreLib 
    pybind11::pybind11 
    pybind11::module 
    pybind11::embed
)
set_target_properties(CorePyExt PROPERTIES
    SUFFIX ${PYTHON_MODULE_EXTENSION}
)

install(
    TARGETS CorePyExt
    LIBRARY DESTINATION ${OUTPUT_DIR}
    ARCHIVE DESTINATION ${OUTPUT_DIR}
)
