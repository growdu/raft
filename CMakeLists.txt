cmake_minimum_required(VERSION 3.0.0)
project(raft VERSION 1.0.0)

set(LIBUV_HOME_DIR $ENV{LIBUV_HOME_DIR})
set(RAFT_HOME_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_definitions(-fvisibility=hidden)

add_subdirectory(src)
add_subdirectory(example)