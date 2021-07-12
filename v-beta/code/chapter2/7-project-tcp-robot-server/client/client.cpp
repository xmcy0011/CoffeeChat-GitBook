#include <iostream>

#include <cerrno>
#include <thread>
#include <sys/socket.h> // bind,connect
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h>  // inet_addr()
#include <unistd.h>     // close

const int kSocketError = -1;

int main() {
    // 创建socket
    int fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == kSocketError) {
        std::cout << "socket error:" << errno << std::endl;
        return 0;
    }

    struct sockaddr_in serverIp{};
    serverIp.sin_family = AF_INET;
    serverIp.sin_port = htons(8087);
    serverIp.sin_addr.s_addr = inet_addr("127.0.0.1");

    // 连接到服务器
    std::cout << "connect remote" << std::endl;
    int ret = ::connect(fd, (sockaddr *) &serverIp, sizeof(serverIp));
    if (ret == kSocketError) {
        std::cout << "connect error:" << errno << std::endl;
        return 0;
    }

    std::cout << "robot_server 连接成功，开始和机器人聊天，请输入任意内容(输入exit回车后退出)：" << std::endl;
    std::string input;
    char recvBuffer[1024] = {0};
    while (true) {
        std::cin >> input; // 等待用户输入
        if (input == "exit") {
            break;
        }

        // 发送
        ret = ::send(fd, input.c_str(), input.length(), 0);
        if (ret == kSocketError) {
            std::cout << "send error:" << errno << std::endl;
            break;
        }

        // 阻塞，直到服务器返回数据
        ret = ::recv(fd, recvBuffer, sizeof(recvBuffer), 0);
        if (ret == kSocketError) {
            std::cout << "send error:" << errno << std::endl;
            break;
        }
        std::cout << "AI:" << std::string(recvBuffer, ret) << std::endl;
    }

    // 关闭socket的两端，关闭后，如服务的recv()阻塞会立即返回0，标志客户端的连接已端开
    ::close(fd);

    return 0;
}