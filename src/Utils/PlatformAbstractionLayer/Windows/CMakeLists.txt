cmake_minimum_required(VERSION 3.16.3)

project(WadoEngine VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(WadoPALDir "..")

add_library(WadoWindowsLib Atomics.cpp Fiber.cpp System.cpp Thread.cpp)

target_include_directories(WadoWindowsLib PUBLIC "${WadoPALDir}")