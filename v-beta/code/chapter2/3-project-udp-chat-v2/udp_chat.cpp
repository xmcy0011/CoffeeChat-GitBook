#include <iostream>

#include <sys/socket.h> // bind,recvfrom,close
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h>  // htons
#include <unistd.h>    // close

#include <atomic> // 原子操作
#include <thread> // std thread

#include <vector>

#include "protocol.h"
#include "udp_server.h"

const std::string kListenIp = "0.0.0.0";

/** @fn split
  * @brief 分割字符串
  * @param [in]s: 字符串
  * @param [in]tokens: 结果
  * @param [in]delimiters: 分割符
  * @return
  */
void split(const std::string &s, std::vector<std::string> &tokens, const std::string &delimiters = "") {
    std::string::size_type lastPos = s.find_first_not_of(delimiters, 0);
    std::string::size_type pos = s.find_first_of(delimiters, lastPos);
    while (std::string::npos != pos || std::string::npos != lastPos) {
        tokens.emplace_back(s.substr(lastPos, pos - lastPos)); // C++11
        lastPos = s.find_first_not_of(delimiters, pos);
        pos = s.find_first_of(delimiters, lastPos);
    }
}

/** @fn sigint
  * @brief 获取SIGINT信号
  * @return void
  */
void sigint(int value) {
    std::cout << "SIGINT, program exit" << std::endl;

    UdpServer::getInstance()->stop();
    exit(0);
}

int main() {
    signal(SIGINT, sigint); // 捕获ctrl+c信号

    ::srand(time(nullptr)); // 随机数种子
    int random_port = rand() % UINT16_MAX + 3000; // 随机获取1个端口号

    std::cout << "your port is: " << random_port << std::endl;

    // 初始化接收socket
    if (!UdpServer::getInstance()->listen(kListenIp, random_port)) {
        return 0;
    }

    // 内部启动一个线程，不停接收消息
    UdpServer::getInstance()->run();

    // 主线程接收用户输入
    int fd = UdpServer::getInstance()->listenFd(); // 复用
    while (true) {
        char input[200] = {};
        std::cout << "请输入要发送的内容，格式为 [IP] [Port] [文本内容]，以空格隔开，回车结束，输入exit，退出程序" << std::endl;
        std::cin.getline(input, sizeof(input), '\n');

        std::string input_str(input);
        if (input_str == "exit") {
            break;
        }

        // 格式校验
        std::vector<std::string> arr = {};
        split(input, arr, " ");
        if (arr.size() < 3) {
            std::cout << "错误的格式" << std::endl;
            continue;
        }

        // 解析端口
        int remote_port = atoi(arr[1].c_str());
        if (remote_port <= 0 || remote_port >= UINT16_MAX) {
            std::cout << "错误的端口" << std::endl;
            continue;
        }

        // IP
        struct sockaddr_in dest_addr = {0};
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(remote_port);
        dest_addr.sin_addr.s_addr = inet_addr(arr[0].c_str());

        // 从第3位开始，都是要发送的内容，因为内容有可能有空格，所以要特殊处理以下
        std::string text = input_str.substr(arr[0].length() + arr[1].length() + 2, input_str.length());

        Message data{};
        data.type = static_cast<int>(MsgType::kMsgData);
        assert(text.length() < sizeof(data.data));
        ::memcpy(data.data, text.c_str(), text.length());

        int ret = ::sendto(fd, &data, sizeof(data), 0, (struct sockaddr *) &dest_addr, sizeof(dest_addr));
        if (ret == -1) {
            std::cout << "sendto error: " << errno << std::endl;
            break;
        } else {
            std::cout << "already send" << std::endl; // 为什么不能打印success send？udp并不能保证我们的消息对方一定能收到
        }
    }

    UdpServer::getInstance()->stop();
    std::cout << "exit ..." << std::endl;
    return 0;
}