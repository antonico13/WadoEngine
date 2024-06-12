cmake_minimum_required(VERSION 3.16.3)

project(WadoRenderGraph VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(wadoGALSource "../../GraphicsAbstractionLayer")

add_library(WadoRenderGraph RenderGraph.cpp)

target_include_directories(WadoRenderGraph PUBLIC
                          "${wadoGALSource}"
                          )