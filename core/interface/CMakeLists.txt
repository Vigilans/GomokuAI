cmake_minimum_required (VERSION 3.8)

project(CoreInterface)

add_executable(CoreInterface src/main.cpp)
target_link_libraries(CoreInterface PRIVATE CoreLib)

install(TARGETS CoreInterface RUNTIME DESTINATION ${OUTPUT_DIR})
