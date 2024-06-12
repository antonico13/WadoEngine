cmake_minimum_required(VERSION 3.16.3)

project(QueueTest VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(googleTestSrc "../../../lib/googletest")
set(wadoRDGSource "../../../src/Rendering/RenderGraph")
set(wadoGALSource "../../../src/GraphicsAbstractionLayer")

enable_testing()

add_subdirectory(${googleTestSrc} ${googleTestSrc}/build)
add_subdirectory(${wadoRDGSource} ${wadoRDGSource}/build)


add_executable(RenderGraphTest RenderGraphTests.cpp)

target_link_libraries(RenderGraphTest PUBLIC WadoRenderGraph)
target_link_libraries(RenderGraphTest PUBLIC gtest_main)

target_include_directories(RenderGraphTest PUBLIC
                          "${wadoRDGSource}"
                          "."
                          "${wadoGALSource}"
                          )

include(GoogleTest)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
gtest_discover_tests(RenderGraphTest)

