cmake_minimum_required(VERSION 3.16.3)

project(WadoGAL VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(wadoPtrSource "../Utils")

add_library(WadoGAL WdLayer.cpp WdPipeline.cpp)

target_include_directories(WadoGAL PUBLIC
                          "${wadoPtrSource}"
                          "."
                          )