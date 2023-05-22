cmake_minimum_required(VERSION 3.0)
project(app)

find_package(glfw3 REQUIRED FATAL_ERROR)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)

message(STATUS "Using " $ENV{VULKAN_SDK})

add_subdirectory(src)