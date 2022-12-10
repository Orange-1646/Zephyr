cmake_minimum_required(VERSION 3.20.0 FATAL_ERROR)

project(ZephyrApp)

file(GLOB ZEPHYR_APP_SRC main.cpp ZephyrApp.cpp)

add_executable(ZephyrApp ${ZEPHYR_APP_SRC})
target_include_directories(ZephyrApp PUBLIC ${ZEPHYR_RUNTIME_INCLUDE_DIR})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON FATAL)
set(CMAKE_CXX_EXTENSIONS OFF)