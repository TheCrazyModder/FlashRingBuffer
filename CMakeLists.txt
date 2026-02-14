cmake_minimum_required(VERSION 4.1.2)

project(ring_buffer C)

option(USE_PC_SIM "Build with PC simulated HAL" OFF)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

if(USE_PC_SIM)
    message(STATUS "Building with PC simulated HAL")
    list(FILTER SRC_FILES EXCLUDE REGEX ".*/src/real/.*\\.c$")
else()
    message(STATUS "Building with hardware HAL")
    list(FILTER SRC_FILES EXCLUDE REGEX ".*/src/pc_sim/.*\\.c$")
endif()

add_library(ring_buffer STATIC ${SRC_FILES})

target_include_directories(ring_buffer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/include
)

add_executable(out src/tests/test_basic.c)
target_link_libraries(out PRIVATE ring_buffer)

# add_executable(ring_buffer ${SRC_FILES})