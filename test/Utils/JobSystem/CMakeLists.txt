cmake_minimum_required(VERSION 3.16.3)

project(JobTest VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(WadoWindowsDir "../../../src/Utils/PlatformAbstractionLayer/Windows/")
set(WadoFiberSystemDir "../../../src/Utils/FiberSystem/")
set(WadoTaskSystemDir "../../../src/Utils/TaskSystem/")
set(GoogleBenchmarkDir "../../../lib/benchmark/")

add_subdirectory(${WadoWindowsDir} build/windowsLib)
add_subdirectory(${WadoFiberSystemDir} build/fiberSystem)
add_subdirectory(${WadoTaskSystemDir} build/taskSystem)
add_subdirectory(${GoogleBenchmarkDir} build/benchmark)


add_executable(JobTest main.cpp)

target_link_libraries(JobTest PUBLIC WadoWindowsLib WadoFiberSystem WadoTaskSystem benchmark::benchmark)

target_include_directories(JobTest PUBLIC
                          "${WadoTaskSystemDir}"
                          "${WadoFiberSystemDir}"
                          )


