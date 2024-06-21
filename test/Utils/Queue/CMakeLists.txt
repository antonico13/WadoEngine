cmake_minimum_required(VERSION 3.16.3)

project(QueueTest VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(googleTestSrc "../../../lib/googletest")
set(wadoQueueSrc "../../../src/Utils/Queue")

enable_testing()

add_subdirectory(${googleTestSrc} ${googleTestSrc}/build)

add_executable(QueueTest QueueTests.cpp)

target_link_libraries(QueueTest PUBLIC gtest_main)

target_include_directories(QueueTest PUBLIC
                          "${wadoQueueSrc}"
                          )

include(GoogleTest)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
gtest_discover_tests(QueueTest)

