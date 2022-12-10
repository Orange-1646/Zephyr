cmake_minimum_required(VERSION 3.20.0 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON FATAL)
set(CMAKE_CXX_EXTENSIONS OFF)

project(ZephyrTest)

file(GLOB_RECURSE SRC src/*.cpp)

foreach(file ${SRC})
	get_filename_component(fileName ${file} NAME_WE)
	add_executable(${fileName} ${file})
	target_include_directories(${fileName} PUBLIC ${ZEPHYR_RUNTIME_INCLUDE_DIR})
	target_link_libraries(${fileName} ZephyrRuntime)
endforeach(file ${SRC})
