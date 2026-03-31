cmake_minimum_required(VERSION 3.15)
project(lab_3)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)


set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)


add_library(array_lib
    src/array.cpp
)
target_include_directories(array_lib PUBLIC src)

enable_testing()

add_executable(lab_3_tests
    tests/test.cpp
)

add_executable(lab_3
    src/main.cpp
    src/marker.cpp
)

target_link_libraries(lab_3 PRIVATE array_lib)

target_compile_options(lab_3_tests PRIVATE /Y-)

target_link_libraries(lab_3_tests
    PRIVATE
    array_lib       
    GTest::gtest
    GTest::gtest_main  
)


include(GoogleTest)
gtest_discover_tests(lab_3_tests)