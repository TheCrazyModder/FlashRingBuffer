cmake_minimum_required(VERSION 4.2.3)

project(ring_buffer C)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

add_executable(ring_buffer ${SRC_FILES})