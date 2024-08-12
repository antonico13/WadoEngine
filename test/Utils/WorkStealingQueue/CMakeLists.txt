cmake_minimum_required(VERSION 3.16.3)

project(WSQueueTest VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(googleTestSrc "../../../lib/googletest")
set(WadoPALDir "../../../src/Utils/PlatformAbstractionLayer/")
set(WadoWindowsDir "../../../src/Utils/PlatformAbstractionLayer/Windows/")
set(WadoCircularArraySrc "../../../src/Utils/Containers")
set(WadoQueueSrc "../../../src/Utils/Queue")

enable_testing()

add_subdirectory(${WadoWindowsDir} build/windowsLib)
add_subdirectory(${googleTestSrc} ${googleTestSrc}/build)

add_executable(WSQueueTest WSQueueTest.cpp)

target_link_libraries(WSQueueTest PUBLIC gtest_main WadoWindowsLib)

target_include_directories(WSQueueTest PUBLIC
                          "${WadoQueueSrc}"
                          "${WadoPALDir}"
                          "${WadoCircularArraySrc}"
                          )

include(GoogleTest)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
gtest_discover_tests(WSQueueTest)

