cmake_minimum_required(VERSION 3.16.3)

project(FiberTest VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(googleTestSrc "../../../lib/googletest")
set(WadoWindowsDir "../../../src/Utils/PlatformAbstractionLayer/Windows/")
set(WadoFiberSystemDir "../../../src/Utils/FiberSystem/")
set(WadoTaskSystemDir "../../../src/Utils/TaskSystem/")

enable_testing()

add_subdirectory(${WadoWindowsDir} build/windowsLib)
add_subdirectory(${WadoFiberSystemDir} build/fiberSystem)
add_subdirectory(${WadoTaskSystemDir} build/taskSystem)
add_subdirectory(${googleTestSrc} ${googleTestSrc}/build)

add_executable(FiberTest FiberTest.cpp)

target_link_libraries(FiberTest PUBLIC WadoWindowsLib WadoFiberSystem WadoTaskSystem gtest_main)

target_include_directories(FiberTest PUBLIC
                          "${WadoFiberSystemDir}"
                          "${WadoTaskSystemDir}"
                          )

include(GoogleTest)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
gtest_discover_tests(FiberTest)

