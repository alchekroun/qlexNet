cmake_minimum_required(VERSION 3.31)
project(qlexNet VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define an INTERFACE library (header-only)
add_library(${PROJECT_NAME} INTERFACE)

# Expose the include directory
target_include_directories(
    ${PROJECT_NAME} INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/extern/asio-1.30.2/include"
    )
