cmake_minimum_required(VERSION 3.21)
project(test_cases)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -fcoroutines")

# Get real root of the project
get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

find_package(GTest REQUIRED)
find_package(Threads REQUIRED)
find_package(mongocxx REQUIRED)
find_package(bsoncxx REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(spdlog REQUIRED)


message(STATUS "PROJECT_ROOT = ${PROJECT_ROOT}")

# Add files from the core directory
file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS
    ${PROJECT_ROOT}/core/*.h
    ${PROJECT_ROOT}/core/*.hpp
)
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    ${PROJECT_ROOT}/core/*.c
    ${PROJECT_ROOT}/core/*.cpp
)

enable_testing()

add_executable(${PROJECT_NAME}
    test_main.cpp
    ${SRC_FILES}
    ${HEADER_FILES}
)

# Include directories
target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${PROJECT_ROOT}/core
)

target_link_libraries(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} PRIVATE mongo::bsoncxx_shared)
target_link_libraries(${PROJECT_NAME} PRIVATE mongo::mongocxx_shared)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::Crypto OpenSSL::SSL)
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog)
target_link_libraries(${PROJECT_NAME} PRIVATE GTest::gtest GTest::gtest_main Threads::Threads)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})