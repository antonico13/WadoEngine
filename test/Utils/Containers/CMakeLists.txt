cmake_minimum_required(VERSION 3.16.3)

project(CircularArrayTest VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(googleTestSrc "../../../lib/googletest")
set(wadoCircularArraySrc "../../../src/Utils/Containers")

enable_testing()

add_subdirectory(${googleTestSrc} ${googleTestSrc}/build)

add_executable(CircularArrayTest CircularArrayTest.cpp)

target_link_libraries(CircularArrayTest PUBLIC gtest_main)

target_include_directories(CircularArrayTest PUBLIC
                          "${wadoCircularArraySrc}"
                          )

include(GoogleTest)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
gtest_discover_tests(CircularArrayTest)

