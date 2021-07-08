cmake_minimum_required(VERSION 3.17)
project(4_project_udp_chat_v3)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(4_project_udp_chat_v3
        protocol.h
        udp_chat.cpp
        udp_server.cpp
        udp_server.h)
