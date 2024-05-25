cmake_minimum_required(VERSION 3.21)
project(http_server_cpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS -pthread)

find_package(glog REQUIRED)
find_package(mongocxx REQUIRED)
find_package(bsoncxx REQUIRED)
find_package(OpenSSL REQUIRED)

include_directories(${LIBMONGOCXX_INCLUDE_DIR})
include_directories(${LIBBSONCXX_INCLUDE_DIR})
include_directories("/usr/local/lib")
include_directories(core/include)
include_directories(server_app/include)
include_directories(server_app/src)

# Header files
file (GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/core/*.h
    ${PROJECT_SOURCE_DIR}/core/*.hpp
    ${PROJECT_SOURCE_DIR}/server_app/*.h
    ${PROJECT_SOURCE_DIR}/server_app/*.hpp
)

# Source files
file (GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/core/*.c
    ${PROJECT_SOURCE_DIR}/core/*.cpp
    ${PROJECT_SOURCE_DIR}/server_app/*.c
    ${PROJECT_SOURCE_DIR}/server_app/*.cpp
)

# Filter out unwanted files based on a pattern
# list(FILTER SRC_FILES EXCLUDE REGEX ".*_coinbase\\.cpp$|.*coinbase_.*\\.cpp$|.*coinbase\\.cpp$")
list(FILTER SRC_FILES EXCLUDE REGEX "server_app/src/trading_strategy/.*")
#message(STATUS "Filtered SRC_FILES: ${SRC_FILES}")

add_executable(${PROJECT_NAME} ${HEADER_FILES} ${SRC_FILES})

target_link_libraries(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} PRIVATE mongo::bsoncxx_shared)
target_link_libraries(${PROJECT_NAME} PRIVATE mongo::mongocxx_shared)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::Crypto OpenSSL::SSL)
target_link_libraries(${PROJECT_NAME} PRIVATE glog::glog)