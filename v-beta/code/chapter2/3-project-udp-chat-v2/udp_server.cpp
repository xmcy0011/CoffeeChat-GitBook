/** @file udp_server.h
  * @brief 
  * @author teng.qing
  * @date 2021/7/7
  */

#include <sys/socket.h> // bind,recvfrom,close
#include <arpa/inet.h>  // htons
#include <unistd.h>    // close

#include <atomic> // 原子操作
#include <thread> // std thread
#include <iostream>

#include "protocol.h"
#include "udp_server.h"

UdpServer::UdpServer() : listen_fd_(0), recv_thread_run_(true) {

}

UdpServer *UdpServer::getInstance() {
    // 还有饿汉式、懒汉式：https://blog.csdn.net/zhanghuaichao/article/details/79459130

    // C++11推荐写法，线程安全
    // 参考：https://blog.csdn.net/i_chaoren/article/details/80450403、https://www.zhihu.com/question/50533404
    // VS 2013不安全，VS2015以上OK
    static UdpServer instance;
    return &instance;
}

bool UdpServer::listen(const std::string &listen_ip, int port) {
    // 1. 创建Socket
    listen_fd_ = ::socket(AF_INET, SOCK_DGRAM, 0); // create socket
    if (listen_fd_ == -1) {
        std::cout << "create socket error:" << errno << std::endl;
        return false;
    }

    // 2. 绑定IP地址
    struct sockaddr_in address{};
    address.sin_family = AF_INET; // 使用IPv4协议
    address.sin_port = htons(port); // 设置接收方端口号，转换成大端
    address.sin_addr.s_addr = inet_addr(listen_ip.c_str()); //设置接收方IP

    int ret = bind(listen_fd_, (struct sockaddr *) &address, sizeof(address));
    if (ret < 0) {
        std::cout << "bind fail:" << errno << std::endl;
        ::close(listen_fd_);
        return false;
    }
    std::cout << "udp server listen on " << listen_ip << ":" << port << std::endl;
    return true;
}

void UdpServer::run() {
    auto cb = [this] { recvThreadProc(); }; // C++11 lambda
    std::thread t(cb);
    t.detach();
}

void UdpServer::stop() {
    // 标记，线程不继续运行
    recv_thread_run_ = false;

    // 最后，退出前关闭socket文件具柄，线程recvfrom会返回，进而退出
    if (listen_fd_ != 0) {
        ::close(listen_fd_);
        listen_fd_ = 0;
    }
}

void UdpServer::recvThreadProc() {
    // 3. 不停的接收来自于远端的数据
    while (recv_thread_run_) {
        struct sockaddr_in remote_addr = {0}; // 对方的IP和端口
        socklen_t addr_len = sizeof(remote_addr); // socklen_t 是mac上的结构体，如果Linux编译不过，请换成int

        const int kRecvBufferLen = 1024; // 1 KB
        char buffer[kRecvBufferLen] = {0};              // 接收缓冲区
        int recv_len = ::recvfrom(listen_fd_, buffer, kRecvBufferLen, 0, (struct sockaddr *) &remote_addr,
                                  &addr_len);
        if (recv_len == -1) {
            if (errno == EBADF) { // close the fd
            } else {
                std::cout << "unknown error:" << errno << std::endl;
            }
            break;
        }

        // 4. 处理数据
        onHandle(buffer, recv_len, remote_addr);
    }
    std::cout << "recv thread exit." << std::endl;
}

UdpServer::~UdpServer() {
    stop();
}

void UdpServer::onHandle(const char *buffer, int len, struct sockaddr_in &remote_addr) {
    std::string end_point = std::string(inet_ntoa(remote_addr.sin_addr)) + ":" +
                            std::to_string(remote_addr.sin_port);

    Message message{};

    ::memcpy(&message.type, buffer, sizeof(int32_t));
    buffer += sizeof(int32_t); // 注意偏移

    assert(len <= sizeof(message));
    ::memcpy(message.data, buffer, len);

    if (static_cast<MsgType>(message.type) == MsgType::kMsgData) {
        // 4. 收到消息
        std::cout << "来自" << end_point << " " << std::string(message.data) << std::endl;

        // 给对方回复收到
        Message ack{};
        ack.type = static_cast<int32_t>(MsgType::kMsgAck);

        int ret = ::sendto(listenFd(), &ack, sizeof(ack), 0, (struct sockaddr *) &remote_addr,
                           sizeof(remote_addr));
        if (ret == -1) {
            std::cout << "sendto error: " << errno << std::endl;
        }
    } else if (static_cast<MsgType>(message.type) == MsgType::kMsgAck) {
        std::cout << "来自" << end_point << " " << " 收到对方的确认回复" << std::endl;
    } else {
        std::cout << "来自" << end_point << " " << " Unknown message type:" << message.type << std::endl;
    }
}


