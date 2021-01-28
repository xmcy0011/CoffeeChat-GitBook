cmake_minimum_required(VERSION 3.17)
project(_book_code)

set(CMAKE_CXX_STANDARD 14)

if(APPLE)
    #设置自己mac os的openssl的路径
    SET(OPENSSL_INCLUDE_DIR /usr/local/Cellar/openssl@1.1/1.1.1i/include)
    SET(OPENSSL_LIB_DIR /usr/local/Cellar/openssl@1.1/1.1.1i/lib)
endif(APPLE)

#find_package(OpenSSL REQUIRED)
#if(OPENSSL_FOUND)
#    target_include_directories(${OPENSSL_INCLUDE_DIR})
#    target_link_libraries(${OPENSSL_LIB_DIR})
#else(OPENSSL_FOUND)
#    message(FATAL_ERROR ”OPENSSL library not found”)
#endif(OPENSSL_FOUND)

include_directories(${OPENSSL_INCLUDE_DIR})
link_directories(${OPENSSL_LIB_DIR})

add_executable(_book_code main.cpp)
target_link_libraries(_book_code crypto ssl)