cmake_minimum_required(VERSION 3.8)

project(CoreTest)

enable_testing()
find_package(GTest REQUIRED)
include_directories(./)

add_executable(CoreTest
    pch.cpp
    unit/player_unittest.cpp
    unit/position_unittest.cpp
    unit/mcts_unittest.cpp
    integration/board_integrationtest.cpp
)
target_link_libraries(CoreTest PRIVATE 
    CoreLib 
    GTest::GTest 
    GTest::Main
)
add_test(AllTestsInMain CoreTest)

# No need to install tests