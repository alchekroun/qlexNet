cmake_minimum_required(VERSION 3.31)
project(qlexEngine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Var
set(EXTERN_DIR "${CMAKE_SOURCE_DIR}/extern")
set(LIB_DIR "${CMAKE_SOURCE_DIR}/lib")

# Include headers
include_directories(${LIB_DIR})

# Add Asio header directory
include_directories(${EXTERN_DIR}/asio-1.30.2/include)

add_executable(${PROJECT_NAME} "main.cpp")