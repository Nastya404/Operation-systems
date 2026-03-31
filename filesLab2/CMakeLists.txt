cmake_minimum_required(VERSION 3.14)
project(Lab2_Threads LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Main application
add_executable(lab2
    src/main.cpp
    src/threads.cpp
)
target_include_directories(lab2 PRIVATE include)