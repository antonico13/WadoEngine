cmake_minimum_required(VERSION 3.16.3)

project(WadoEngine VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT ProgramDatabase)

set(WadoPALDir "../PlatformAbstractionLayer/")
set(WadoQueueDir "../Queue/")
set(WadoFiberSystemDir "../FiberSystem/")
set(TracyDir "../../../lib/tracy")

add_library(WadoTaskSystem TaskSystem.cpp)

target_include_directories(WadoTaskSystem PUBLIC "${WadoPALDir}" "${WadoQueueDir}" "${TracyDir}/public/" "${WadoFiberSystemDir}" ".")