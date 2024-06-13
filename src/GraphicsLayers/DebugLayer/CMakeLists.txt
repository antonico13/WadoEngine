cmake_minimum_required(VERSION 3.16.3)

project(WadoDebugGAL VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(wadoGALSource "../../GraphicsAbstractionLayer")
set(wadoPtrSource "../../Utils")

add_library(WadoDebugGAL DebugLayer.cpp DebugCommandList.cpp)

target_include_directories(WadoDebugGAL PUBLIC
                          "${wadoGALSource}"
                          "${wadoPtrSource}"
                          "."
                          )