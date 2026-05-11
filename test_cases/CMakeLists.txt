cmake_minimum_required(VERSION 3.21)
project(test_cases)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -fcoroutines")

# Get real root of the project
get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

# Debug output to verify the root path
# message(STATUS "PROJECT_ROOT = ${PROJECT_ROOT}")

find_package(GTest REQUIRED)
find_package(Threads REQUIRED)
find_package(mongocxx REQUIRED)
find_package(bsoncxx REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(spdlog REQUIRED)

include_directories("/usr/local/lib")
include_directories(${LIBMONGOCXX_INCLUDE_DIR})
include_directories(${LIBBSONCXX_INCLUDE_DIR})

# Add files from the core directory
file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS
    ${PROJECT_ROOT}/core/cache/*.h
    ${PROJECT_ROOT}/core/cache/*.hpp
    ${PROJECT_ROOT}/core/json/*.h
    ${PROJECT_ROOT}/core/json/*.hpp
    ${PROJECT_ROOT}/core/network/https_client_request/*.h
    ${PROJECT_ROOT}/core/network/https_client_request/*.hpp
    ${PROJECT_ROOT}/core/network/https_client_websocket/*.h
    ${PROJECT_ROOT}/core/network/https_client_websocket/*.hpp
    ${PROJECT_ROOT}/server_app/pnl/*.h
    ${PROJECT_ROOT}/server_app/pnl/*.hpp
    ${PROJECT_ROOT}/server_app/app_utils/*.h
    ${PROJECT_ROOT}/server_app/app_utils/*.hpp
)
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    ${PROJECT_ROOT}/core/cache/*.c
    ${PROJECT_ROOT}/core/cache/*.cpp
    ${PROJECT_ROOT}/core/json/*.c
    ${PROJECT_ROOT}/core/json/*.cpp
    ${PROJECT_ROOT}/core/network/https_client_request/https_client_response_parser*.cpp
    ${PROJECT_ROOT}/core/network/https_client_request/https_client_chunk_parser*.cpp
    ${PROJECT_ROOT}/core/network/https_client_websocket/websocket_frame_parser.cpp
    ${PROJECT_ROOT}/core/network/https_client_websocket/websocket_frame_builder.cpp
    ${PROJECT_ROOT}/server_app/pnl/*.c
    ${PROJECT_ROOT}/server_app/pnl/*.cpp
    ${PROJECT_ROOT}/server_app/app_utils/*.c
    ${PROJECT_ROOT}/server_app/app_utils/*.cpp
)

# Test sources
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/core/**/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/server_app/*.cpp
)

enable_testing()

add_executable(${PROJECT_NAME}
    ${TEST_SOURCES}
    ${SRC_FILES}
    ${HEADER_FILES}
)

# Include directories
target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${PROJECT_ROOT}/core
    ${PROJECT_ROOT}/server_app
)

target_link_libraries(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} PRIVATE mongo::bsoncxx_shared)
target_link_libraries(${PROJECT_NAME} PRIVATE mongo::mongocxx_shared)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::Crypto OpenSSL::SSL)
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog)
target_link_libraries(${PROJECT_NAME} PRIVATE GTest::gtest GTest::gtest_main Threads::Threads)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})