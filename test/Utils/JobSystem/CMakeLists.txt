cmake_minimum_required(VERSION 3.16.3)

project(JobTest VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT ProgramDatabase)

set(WadoWindowsDir "../../../src/Utils/PlatformAbstractionLayer/Windows/")
set(WadoFiberSystemDir "../../../src/Utils/FiberSystem/")
set(WadoTaskSystemDir "../../../src/Utils/TaskSystem/")
set(TracyDir "../../../lib/tracy")

add_subdirectory(${WadoWindowsDir} build/windowsLib)
add_subdirectory(${WadoFiberSystemDir} build/fiberSystem)
add_subdirectory(${WadoTaskSystemDir} build/taskSystem)

add_definitions(-DTRACY_ENABLE -DTRACY_ONLY_LOCALHOST -DTRACY_FIBERS -DTRACY_ON_DEMAND -DTRACY_NO_EXIT)

add_executable(JobTest main.cpp "${TracyDir}/public/TracyClient.cpp")

target_link_libraries(JobTest PUBLIC WadoWindowsLib WadoFiberSystem WadoTaskSystem)

target_include_directories(JobTest PUBLIC
                          "${WadoTaskSystemDir}"
                          "${WadoFiberSystemDir}"
                          "${TracyDir}/public/"
                          )


