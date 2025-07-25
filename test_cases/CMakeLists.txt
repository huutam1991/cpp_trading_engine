cmake_minimum_required(VERSION 3.21)
project(test_cases)

# Find GTest installed on the system
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

enable_testing()

add_executable(run_tests test_main.cpp)

# Link with GTest and pthread (required on Linux)
target_link_libraries(run_tests GTest::gtest GTest::gtest_main pthread)

# Automatically discover and register test cases
include(GoogleTest)
gtest_discover_tests(run_tests)