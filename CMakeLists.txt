cmake_minimum_required(VERSION 3.20.0 FATAL_ERROR)

project(ZephyrEngine)


set(ZEPHYR_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

add_compile_definitions("ZEPHYR_ROOT_DIR=${ZEPHYR_ROOT_DIR}/")

add_subdirectory(Zephyr)
add_subdirectory(Test)
add_subdirectory(ZephyrApp)