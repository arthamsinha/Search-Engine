cmake_minimum_required(VERSION 3.14)
project(devfinder CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

include_directories(include)

# Gather all source files for the core library
file(GLOB_RECURSE SRC_FILES "src/devfinder/**/*.cpp")
add_library(devfinder_core ${SRC_FILES})
target_link_libraries(devfinder_core PUBLIC CURL::libcurl Threads::Threads)
if(WIN32)
    target_link_libraries(devfinder_core PUBLIC ws2_32 wsock32)
endif()
target_include_directories(devfinder_core PUBLIC include)

# CLI Executable
add_executable(devfinder_cli cli/main_cli.cpp)
target_link_libraries(devfinder_cli PRIVATE devfinder_core)


