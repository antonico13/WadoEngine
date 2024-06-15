cmake_minimum_required(VERSION 3.16.3)

project(WadoEngine VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(WadoPALDir "../PlatformAbstractionLayer/")
set(WadoQueueDir "../Queue/")
set(WadoFiberSystemDir "../FiberSystem/")

add_library(WadoTaskSystem TaskSystem.cpp)

target_include_directories(WadoTaskSystem PUBLIC "${WadoPALDir}" "${WadoQueueDir}" "${WadoFiberSystemDir}" ".")