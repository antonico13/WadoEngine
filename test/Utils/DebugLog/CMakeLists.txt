cmake_minimum_required(VERSION 3.16.3)

project(DebugLogTest VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(googleTestSrc "../../../lib/googletest")
set(WadoWindowsDir "../../../src/Utils/PlatformAbstractionLayer/Windows/")
set(WadoFiberSystemDir "../../../src/Utils/FiberSystem/")
set(WadoDebgLogDir "../../../src/Utils/DebugLog/")

enable_testing()

add_subdirectory(${WadoWindowsDir} build/windowsLib)
add_subdirectory(${WadoFiberSystemDir} build/fiberSystem)
add_subdirectory(${WadoDebgLogDir} build/debugLog)
add_subdirectory(${googleTestSrc} ${googleTestSrc}/build)

add_executable(DebugLogTest DebugLogTest.cpp)

target_link_libraries(DebugLogTest PUBLIC WadoWindowsLib WadoFiberSystem WadoDebugLog gtest_main)

target_include_directories(DebugLogTest PUBLIC
                          "${WadoDebgLogDir}"
                          "${WadoFiberSystemDir}"
                          )

include(GoogleTest)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
gtest_discover_tests(DebugLogTest)

