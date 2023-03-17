cmake_minimum_required(VERSION 3.10...3.26)

project(sunnet C CXX)

# ExternalProject_Add( lua URL ${LUA_URL} CONFIGURE_COMMAND "" BUILD_COMMAND
# make generic BUILD_ALWAYS true BUILD_IN_SOURCE true INSTALL_COMMAND "")
set(LUA_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rd/lua-5.4.4)

include_directories(include)

# ExternalProject_Get_Property(lua SOURCE_DIR)

include_directories(${LUA_PATH}/src)

# 查找./src下所有源文件
aux_source_directory(./src DIR_SRCS)

aux_source_directory(${LUA_PATH}/src LUA_SRCS)

add_definitions(-std=c++11)

find_package(Threads)

# add_library(liblua STATIC IMPORTED)
#
# add_library(dl STATIC IMPORTED)

add_library(lua STATIC ${LUA_SRCS})

set_property(TARGET lua PROPERTY C_STANDARD 99)
# link_directories(${LUA_PATH}/src)

add_executable(${PROJECT_NAME} ${DIR_SRCS})

target_link_libraries(${PROJECT_NAME} lua)

target_link_libraries(${PROJECT_NAME} dl)

target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
