cmake_minimum_required(VERSION 3.20.0 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON FATAL)
set(CMAKE_CXX_EXTENSIONS OFF)
project(ZephyrTool)

file(GLOB ZEPHYR_TOOL_SRC main.cpp ./Common/*.cpp)
file(GLOB ZEPHYR_TOOL_SPECULAR_SRC ./SpecularMapGenerator/*.cpp)

add_library(ZephyrTool ${ZEPHYR_TOOL_SRC})
target_include_directories(ZephyrTool PUBLIC ${ZEPHYR_RUNTIME_INCLUDE_DIR})

add_executable(SpecularMapGenerator ${ZEPHYR_TOOL_SPECULAR_SRC})
target_include_directories(SpecularMapGenerator PUBLIC ${ZEPHYR_RUNTIME_INCLUDE_DIR})
target_link_libraries(SpecularMapGenerator ZephyrRuntime)

