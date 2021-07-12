cmake_minimum_required(VERSION 3.17)
project(tcp-robot-server)

set(CMAKE_CXX_STANDARD 14)

if(APPLE)
    SET(OPENSSL_INCLUDE_DIR /usr/local/Cellar/openssl@1.1/1.1.1j/include)
    SET(OPENSSL_LIB_DIR /usr/local/Cellar/openssl@1.1/1.1.1j/lib)
endif()

include_directories(
        ./cpp-httplib
        ./lohmann-json
        ${OPENSSL_INCLUDE_DIR})
link_directories(${OPENSSL_LIB_DIR})

add_executable(server server.cpp wechat_api.h wechat_api.cpp)
target_link_libraries(server crypto ssl)