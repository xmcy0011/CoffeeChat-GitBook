cmake_minimum_required(VERSION 3.17)
project(protobuf_examples)

set(CMAKE_CXX_STANDARD 14)

include_directories(pb)
include_directories(/usr/local/protobuf3/include) # 执行make-protobuf.sh，后头文件会拷贝到这里

link_directories(/usr/local/protobuf3/lib)  # 包含protobuf3所在库目录

add_executable(protobuf_examples
        pb/chat.base.pb.cc
        pb/chat.base.pb.h
        pb/chat.msg.pb.cc
        pb/chat.msg.pb.h
        main.cpp)

target_link_libraries(protobuf_examples protobuf)