cmake_minimum_required(VERSION 3.17)
project(udp_chat_v2)

set(CMAKE_CXX_STANDARD 14)

add_executable(udp_chat_v2 udp_chat.cpp udp_server.h udp_server.cpp protocol.h)
