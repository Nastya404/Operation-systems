cmake_minimum_required(VERSION 3.16)
project(Lab_4C++98 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.27")
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()

option(BUILD_TESTING "Build tests" ON)

set(PROJECT_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/headers")
include_directories(${PROJECT_INCLUDE_DIR})

add_library(SharedQueue
        src/SharedQueue.cpp
)
target_include_directories(SharedQueue PUBLIC headers)

add_executable(receiver
        src/Receiver.cpp
)
target_link_libraries(receiver PRIVATE SharedQueue)

add_executable(sender
        src/Sender.cpp
)
target_link_libraries(sender PRIVATE SharedQueue)

if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/release-1.7.0.zip
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    if(DEFINED googletest_SOURCE_DIR)
        include_directories(${googletest_SOURCE_DIR}/include)
    endif()

    file(GLOB TEST_SOURCES CONFIGURE_DEPENDS
            "${CMAKE_SOURCE_DIR}/tests/test_*.cpp"
    )

    foreach(test_src ${TEST_SOURCES})
        get_filename_component(test_name "${test_src}" NAME_WE)

        add_executable("${test_name}" "${test_src}"
                tests/test_sender.cpp
                tests/test_reciever.cpp
		tests/test_shared_queue.cpp)
        target_link_libraries("${test_name}"
                PRIVATE
                SharedQueue
                gtest_main
        )
        target_include_directories("${test_name}" PRIVATE "${PROJECT_INCLUDE_DIR}")
        if(DEFINED googletest_SOURCE_DIR)
            target_include_directories("${test_name}" PRIVATE "${googletest_SOURCE_DIR}/googletest/include")
        endif()

        add_test(NAME "${test_name}" COMMAND "${test_name}")
    endforeach()
endif()