cmake_minimum_required(VERSION 3.14)
project(EmployeeServerTests)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(employee_tests
    tests/Test.cpp
    src/Employee.cpp
    src/FileHandler.cpp
    src/LockManager.cpp
)

target_include_directories(employee_tests PRIVATE headers)
target_link_libraries(employee_tests GTest::gtest_main)

enable_testing()
add_test(NAME employee_tests COMMAND employee_tests)