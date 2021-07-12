#include <iostream>

#include <cstring>
#include <cerrno>
#include <netinet/in.h> // ipv4: PF_INET,sockaddr_in ,v6:PF_INET6,sockaddr_in6
#include <sys/socket.h> // socket,bind,listen,accept
#include <unistd.h>     // read,close
#include <arpa/inet.h>  // inet_addr

#include <locale>   // string 和 wstring互转
#include <codecvt>

#include "wechat_api.h"

const int kSocketError = -1;

class TcpServer {
public:
    TcpServer() : listen_fd_(0) {

    }

    ~TcpServer() = default;

    TcpServer(const TcpServer &) = delete;

    TcpServer(const TcpServer &&) = delete;

    TcpServer operator=(const TcpServer &) = delete;

public:
    /** @fn init
      * @brief 初始化并绑定IP
      * @param [in]aa:
      * @return
      */
    bool init(in_addr_t server_ip, int16_t port) {
        // 创建socket，用于与客户端建立连接
        listen_fd_ = ::socket(PF_INET, SOCK_STREAM, 0);
        if (listen_fd_ == kSocketError) {
            std::cout << "create socket error:" << errno << std::endl;
            return false;
        }
        std::cout << "create socket" << std::endl;

        struct sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);  // 转成网络大端序
        addr.sin_addr.s_addr = server_ip;

        // SO_REUSEADDR，复用IP地址，否则下一次启动会提示，48：Address already in use
        int yesReuseAddr = 1;
        if (::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &yesReuseAddr, sizeof(yesReuseAddr)) == kSocketError) {
            std::cout << "setsockopt error:" << errno << std::endl;
            return false;
        }

        // 绑定IP地址和端口
        int ret = ::bind(listen_fd_, (sockaddr *) &addr, sizeof(addr));
        if (ret == kSocketError) {
            std::cout << "bind socket error:" << errno << std::endl;
            return false;
        }

        std::cout << "bind success,start listen..." << std::endl;
        // 监听listenFd，本质是标识文件描述符为被动socket
        ret = ::listen(listen_fd_, SOMAXCONN);
        if (ret == kSocketError) {
            std::cout << "listen error:" << errno << std::endl;
            return false;
        }
        return true;
    }

    /** @fn run
      * @brief 无限循环，提供echo服务。单线程方式，同一时间只能有一个客户端连接上来
      * @param [in]aa:
      * @return
      */
    void run() {
        // 死循环，永不退出,第一层while用于处理等待连接，产生新的文件描述符
        //新的描述符用于第二层while中的通信
        while (true) {
            struct sockaddr_in peerAddr{};
            socklen_t sockLen = sizeof(sockaddr_in);
            // 接受新的连接，会一直阻塞，直到新连接的到来。
            int fd = ::accept(listen_fd_, (sockaddr *) &peerAddr, &sockLen);
            if (fd == kSocketError) {
                std::cout << "server close" << std::endl;
                return;
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
                    std::cout << "remote close the connection " << std::endl;
                    break;

                } else { //这里是一个echo服务，即服务端向客户端回复同样的内容
                    std::cout << "recv: " << buffer << ",len=" << len << std::endl;

                    onHandle(fd, buffer, len);
                }
            }

            // 关闭socket，shutdown可以指定在某个方向上终止连接
            ::close(fd);
            std::cout << "remote " << ::inet_ntoa(peerAddr.sin_addr) << " disconnect" << std::endl;
        }
    }

private:
    /** @fn s2ws
      * @brief utf8 string convert to wstring
      * @param [in]str: utf8 string
      * @return utf8 wstring
      */
    std::wstring s2ws(const std::string &str) {
        using convert_typeX = std::codecvt_utf8<wchar_t>;
        std::wstring_convert<convert_typeX, wchar_t> converterX;

        return converterX.from_bytes(str);
    }

    /** @fn ws2s
      * @brief utf8 wstring convert to string
      * @param [in]str: utf8 wstring
      * @return utf8 string
      */
    std::string ws2s(const std::wstring &w_str) {
        using convert_typeX = std::codecvt_utf8<wchar_t>;
        std::wstring_convert<convert_typeX, wchar_t> converterX;

        return converterX.to_bytes(w_str);
    }

    /** @fn replace_all
      * @brief 字符串替换
      */
    void replace_all(std::wstring &origin, const std::wstring &mark, const std::wstring &replacement = L"") {
        std::size_t pos = origin.find(mark);
        while (pos != std::string::npos) {
            origin.replace(pos, mark.length(), replacement);
            pos = origin.find(mark);
        }
    }

    /** @fn simple_ai
      * @brief 价值1个亿的AI代码
      */
    std::string simple_ai(std::string &text) {
        std::wstring str = s2ws(text);  // 转换成宽字符，一个字符占4个字节

        replace_all(str, L"吗");
        replace_all(str, L"?", L"!");
        replace_all(str, L"？", L"!"); // 全角问号

        return ws2s(str);// 注意，再转换回来
    }

    /** @fn getAnswer
      * @brief 获取回答
      */
    std::string getAnswer(std::string &text, int type = 1) {
        if (type == 1) {
            return simple_ai(text);
        }
        std::string answer;
        if (WeChatApi::getAnswer(text, answer)) {
            return answer;
        }
        return "机器人出错啦，请过一会试试呢";
    }

    void onHandle(int fd, char *buffer, int len) {
        std::string text(buffer, len);

        enum {
            Simple = 1,
            UseWeChat = 2,
        };

        std::string answer = getAnswer(text, UseWeChat);

        // echo
        len = send(fd, answer.c_str(), answer.length(), 0);
        if (len == kSocketError) {
            std::cout << "send error:" << errno << std::endl;
        }
    }

private:
    int listen_fd_;
};

/** @fn main
  * @brief 演示socket的基础调用demo，使用了默认同步I/O阻塞+单线程的方式，
  * 即同时只能处理1个连接，直到这个连接断开后才能处理下一个连接。
  * @return
  */
int main() {
//    std::wstring str = L"你好吗？好吗?";
//    replace_all(str, L"吗");
//    replace_all(str, L"?", L"!");
//    replace_all(str, L"？", L"!"); // 全角问号
//    std::cout << ws2s(str) << std::endl;

//    std::string answer;
//    WeChatApi::getAnswer("你好啊小微", answer);

    TcpServer server;
    //server.init(inet_addr("127.0.0.1"), 8088);
    if (server.init(INADDR_ANY, 8087)) {
        server.run();
    }
    return 0;
}