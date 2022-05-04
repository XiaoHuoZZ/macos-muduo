cmake_minimum_required(VERSION 3.21)
project(macos_muduo)

set(CMAKE_CXX_STANDARD 17)

#搜索头文件目录
include_directories(muduo/base)
include_directories(${PROJECT_SOURCE_DIR})

#设置静态库输出目录
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

add_subdirectory(test)
add_subdirectory(muduo/net)
add_subdirectory(muduo/base)

