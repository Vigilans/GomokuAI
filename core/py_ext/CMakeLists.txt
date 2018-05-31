cmake_minimum_required(VERSION 3.8)

project(CorePyExt)

find_package(pybind11 CONFIG REQUIRED)
include_directories(./ ${PYTHON_INCLUDE_DIRS})

add_library(CorePyExt MODULE pch.cpp src/module.cpp)
target_link_libraries(CorePyExt PRIVATE 
    CoreLib
    pybind11::module
)
set_target_properties(CorePyExt PROPERTIES
    PREFIX "${PYTHON_MODULE_PREFIX}"
    SUFFIX "${PYTHON_MODULE_EXTENSION}"
)

install(
    TARGETS CorePyExt
    LIBRARY DESTINATION ${OUTPUT_DIR}
    ARCHIVE DESTINATION ${OUTPUT_DIR}
)
