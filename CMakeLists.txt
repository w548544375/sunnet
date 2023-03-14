project(Sunnet)

cmake_minimum_required(VERSION 2.8)

include_directories(include)

# 查找./src下所有源文件
aux_source_directory(./src DIR_SRCS)

add_definitions(-std=c++11)

find_package(Threads)

add_executable(Sunnet ${DIR_SRCS})

target_link_libraries(Sunnet ${CMAKE_THREAD_LIBS_INIT})
