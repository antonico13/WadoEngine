cmake_minimum_required(VERSION 3.16.3)

project(WadoEngine VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

configure_file(WadoVersion.h.in WadoVersion.h)

add_executable(WadoEngine main.cpp)

option(USE_STB "Use stb_image for texture loading" ON)

if (USE_STB) 
    target_compile_definitions(WadoEngine PUBLIC USE_STB)
endif()

add_subdirectory(lib)
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
#[[find_package(GLM REQUIRED)]]
target_link_libraries(WadoEngine PUBLIC Vulkan::Vulkan glfw)
target_include_directories(WadoEngine PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}/lib/include")
#[[target_link_libraries(WadoEngine PUBLIC TINY_OBJ STB_IMAGE)]]

