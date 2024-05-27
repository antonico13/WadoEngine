cmake_minimum_required(VERSION 3.16.3)

project(ECSTest VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(googleTestSrc "../../../lib/googletest")
set(wadoECSSrc "../../../src/Utils/ECS")

enable_testing()

add_subdirectory(${googleTestSrc} ${googleTestSrc}/build)

add_executable(ECSTest ECSTests.cpp)

target_link_libraries(ECSTest PUBLIC gtest_main)

target_include_directories(ECSTest PUBLIC
                          "${wadoECSSrc}"
                          )

include(GoogleTest)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
gtest_discover_tests(ECSTest)

