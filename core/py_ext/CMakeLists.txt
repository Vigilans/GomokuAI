cmake_minimum_required (VERSION 3.6)

project (CorePyExt)

find_package (pybind11 REQUIRED)  # or add_subdirectory(pybind11)
include_directories(./)

add_library (CorePyExt MODULE src/module.cpp)
target_link_libraries(CorePyExt PRIVATE pybind11::module)
set_target_properties(CorePyExt PROPERTIES 
    PREFIX "${PYTHON_MODULE_PREFIX}"
    SUFFIX "${PYTHON_MODULE_EXTENSION}"
)
