cmake_minimum_required(VERSION 3.16.3)

project(MallocTest VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(wadoMallocSrc "../../../src/Utils/WdMalloc")

add_executable(MallocTest WdMallocTests.cpp)

add_subdirectory(${wadoMallocSrc} "./WdMalloc/build")

target_link_libraries(MallocTest PUBLIC WadoMalloc)

target_include_directories(MallocTest PUBLIC
                          "${wadoMallocSrc}"
                          )

