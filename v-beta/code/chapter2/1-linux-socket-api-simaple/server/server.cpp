#include <iostream>

#include <cstring>
#include <cerrno>
#include <netinet/in.h> // ipv4: PF_INET,sockaddr_in ,v6:PF_INET6,sockaddr_in6
#include <sys/socket.h> // socket,bind,listen,accept
#include <unistd.h>     // read,close
#include <arpa/inet.h>  // inet_addr

const int kSocketError = -1;

/** @fn main
  * @brief 演示socket的基础调用demo，使用了默认同步I/O阻塞+单线程的方式，
  * 即同时只能处理1个连接，直到这个连接断开后才能处理下一个连接。
  * @return
  */

// #define ERROR_CHECK(ret, num, msg){if(ret==num)\
     {perror(msg);return -1;}}
//perror 会自动打印错误信息，
//而error需使用std::cout输出流输出，手动选择是否输出 错误信息
//对于不严谨的宏，最好不要使用，或在说宏最好不要使用

int main() {
    // 创建socket，用于与客户端建立连接
    int listenFd = ::socket(PF_INET, SOCK_STREAM, 0);
    if (listenFd == kSocketError) {
        std::cout << "create socket error:" << errno << std::endl;
        return 0;
    }
    std::cout << "create socket" << std::endl;

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8088);  // 转成网络大端序
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // 将listenFd绑定到本机回环地址的8088端口
    int ret = ::bind(listenFd, (sockaddr *) &addr, sizeof(addr));
    if (ret == kSocketError) {
        std::cout << "bind socket error:" << errno << std::endl;
        return 0;
    }

    std::cout << "bind success,start listen..." << std::endl;
    // 监听listenFd，并且最大监听的连接数为128,本质是标识文件描述符为被动socket
    ret = ::listen(listenFd, SOMAXCONN);
    if (ret == kSocketError) {
        std::cout << "listen error:" << errno << std::endl;
        return 0;
    }

    // 死循环，永不退出,第一层while用于处理等待连接，产生新的文件描述符
    //新的描述符用于第二层while中的通信
    while (true) {
        struct sockaddr_in peerAddr{};
        socklen_t sockLen = sizeof(sockaddr_in);
        // 接受新的连接，会一直阻塞，直到新连接的到来。
        int fd = ::accept(listenFd, (sockaddr *) &peerAddr, &sockLen);
        if (fd == kSocketError) {
            return 0;
        }
        std::cout << "new connect coming,accept..." << std::endl;
        while (true) { // 注意：这里是一个死循环，通常实际中不会这样干
            char buffer[1024] = {};
            // 没有数据时会阻塞,ssize_t是有符号整型,size_t为无符号整型,32位
            ssize_t len = recv(fd, buffer, sizeof(buffer), 0); // wait
            if (len == kSocketError) {
                std::cout << "recv error:" << errno << std::endl;
                break;

            } else if (len == 0) { // 返回0代表对端关闭了连接
                std::cout << "recv error:" << errno << std::endl;
                break;

            } else {   //这里是一个echo服务，即服务端向客户端回复同样的内容
                std::cout << "recv: " << buffer << ",len=" << len << std::endl;
                // echo
                len = send(fd, buffer, len, 0);
                if (len == kSocketError) {
                    std::cout << "send error:" << errno << std::endl;
                    break;
                }
            }
        }

        // 关闭socket，shutdown可以指定在某个方向上终止连接
        ::close(fd);
        std::cout << "remote " << ::inet_ntoa(peerAddr.sin_addr) << "close connection" << std::endl;
    }

    return 0;
}