cmake_minimum_required(VERSION 3.8)

project(CoreLib)

add_library(CoreLib STATIC 
    src/Game.cpp 
    src/MCTS.cpp
    src/Evaluator.cpp
)

include_directories(${EIGEN3_INCLUDE_DIR})
target_link_libraries(CoreLib PRIVATE Eigen3::Eigen)

install(
    TARGETS CoreLib
    LIBRARY DESTINATION ${OUTPUT_DIR}
    ARCHIVE DESTINATION ${OUTPUT_DIR}
)
