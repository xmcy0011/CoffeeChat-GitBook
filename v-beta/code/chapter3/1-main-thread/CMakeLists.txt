cmake_minimum_required(VERSION 3.17)
project(main_thread)

set(CMAKE_CXX_STANDARD 14)

add_executable(main_thread main.cpp)

target_link_libraries(main_thread pthread)