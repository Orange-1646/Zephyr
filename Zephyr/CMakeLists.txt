cmake_minimum_required(VERSION 3.20.0 FATAL_ERROR)

project(ZephyrRuntime)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON FATAL)
set(CMAKE_CXX_EXTENSIONS OFF)


file(GLOB_RECURSE ZEPHYR_RUNTIME_SRC ./src/*.cpp)

find_package(Vulkan REQUIRED FATAL_ERROR)

set(ZEPHYR_RUNTIME_VENDOR_DIR ${PROJECT_SOURCE_DIR}/vendor)
set(ZEPHYR_RUNTIME_SRC_DIR ${PROJECT_SOURCE_DIR}/src)

add_subdirectory(${ZEPHYR_RUNTIME_VENDOR_DIR}/glfw)
add_subdirectory(${ZEPHYR_RUNTIME_VENDOR_DIR}/SPIRV-Cross)
add_subdirectory(${ZEPHYR_RUNTIME_VENDOR_DIR}/assimp)

add_library(ZephyrRuntime STATIC ${ZEPHYR_RUNTIME_SRC} )

target_precompile_headers(ZephyrRuntime PUBLIC ${ZEPHYR_RUNTIME_SRC_DIR}/pch.h)


target_include_directories(ZephyrRuntime PUBLIC ${ZEPHYR_RUNTIME_SRC_DIR})
target_include_directories(ZephyrRuntime PUBLIC ${ZEPHYR_RUNTIME_VENDOR_DIR}/SPIRV-Cross)
target_include_directories(ZephyrRuntime PUBLIC ${ZEPHYR_RUNTIME_VENDOR_DIR}/glfw/include)
target_include_directories(ZephyrRuntime PUBLIC ${ZEPHYR_RUNTIME_VENDOR_DIR}/glm)
target_include_directories(ZephyrRuntime PUBLIC ${ZEPHYR_RUNTIME_VENDOR_DIR}/assimp/include)
target_include_directories(ZephyrRuntime PUBLIC ${ZEPHYR_RUNTIME_VENDOR_DIR}/spdlog/include)
target_include_directories(ZephyrRuntime PUBLIC ${ZEPHYR_RUNTIME_VENDOR_DIR})
target_include_directories(ZephyrRuntime PUBLIC ${ZEPHYR_RUNTIME_VENDOR_DIR}/stb)

set(ZEPHYR_RUNTIME_INCLUDE_DIR ${ZEPHYR_RUNTIME_SRC_DIR} CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)

target_link_libraries(ZephyrRuntime PUBLIC Vulkan::Vulkan 
										   glfw 
										   spirv-cross-core 
										   spirv-cross-glsl
										   spirv-cross-cpp
										   assimp)