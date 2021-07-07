#include <iostream>

#include <sys/socket.h> // bind,recvfrom,close
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h>  // htons
#include <unistd.h>    // close

#include <atomic> // 原子操作
#include <thread> // std thread

#include <vector>

const std::string kListenIp = "0.0.0.0";

std::atomic_bool g_recv_thread_run_(true); // thread run flag
int g_listen_fd = 0;

/** @fn init_server_socket
  * @brief 初始化socket并且绑定IP
  * @param [in]listen_ip: IP
  * @param [in]port: 端口
  * @param [out]out_fd: socket句柄
  * @return 是否成功
  */
bool init_server_socket(const std::string &listen_ip, uint16_t port, int &out_fd) {
    // 1. 创建Socket
    out_fd = ::socket(AF_INET, SOCK_DGRAM, 0); // create socket
    if (out_fd == -1) {
        std::cout << "create socket error:" << errno << std::endl;
        return false;
    }

    // 2. 绑定IP地址
    struct sockaddr_in address{};
    address.sin_family = AF_INET; // 使用IPv4协议
    address.sin_port = htons(port); // 设置接收方端口号，转换成大端
    address.sin_addr.s_addr = inet_addr(listen_ip.c_str()); //设置接收方IP，

    int ret = bind(out_fd, (struct sockaddr *) &address, sizeof(address));
    if (ret < 0) {
        std::cout << "bind fail:" << errno << std::endl;
        ::close(out_fd);
        return false;
    }
    std::cout << "udp server listen on " << listen_ip << ":" << port << std::endl;

    return true;
}

/** @fn recv_thread_proc
  * @brief 接收数据线程
  * @param [in]listen_fd: 监听socket句柄
  * @return
  */
void recv_thread_proc(int listen_fd) {
    // 3. 不停的接收来自于远端的数据
    while (g_recv_thread_run_) {
        struct sockaddr_in remote_addr = {0}; // 对方的IP和端口，即远程的ip和port
        socklen_t addr_len = sizeof(remote_addr); // socklen_t 是mac上的结构体，如果Linux编译不过，请换成int

        const int kRecvBufferLen = 1024; // 1 KB
        char buffer[kRecvBufferLen] = {0};              // 接收缓冲区
        int recv_len = ::recvfrom(listen_fd, buffer, kRecvBufferLen, 0, (struct sockaddr *) &remote_addr, &addr_len);
        if (recv_len == -1) {
            if (errno == EBADF) { // close the fd
            } else {
                std::cout << "unknown error:" << errno << std::endl;
            }
            break;
        }

        // 4. 打印,结构体remote_addr中，存的是远程地址的ip和port，
        // 通过remote_addr.sin_addr 和 remote_addr.sin_port来获取ip和port
        std::cout << "来自" << inet_ntoa(remote_addr.sin_addr) << ":" << remote_addr.sin_port << " "
                  << std::string(buffer) << std::endl;
    }
    std::cout << "recv thread exit." << std::endl;
}

/** @fn clean
  * @brief 退出清理
  * @param [in]listen_fd: 监听fd
  * @return void
  */
void clean(int listen_fd) {
    // 标记，线程不继续运行
    g_recv_thread_run_ = false;

    // 最后，退出前关闭socket文件具柄，线程recvfrom会返回，进而退出
    ::close(listen_fd);
}

/** @fn split
  * @brief 分割字符串
  * @param [in]s: 字符串
  * @param [in]tokens: 结果
  * @param [in]delimiters: 分割符
  * @return
  */
void split(const std::string &s, std::vector<std::string> &tokens, const std::string &delimiters = "") {
    //从头开始找到分隔符后的第一个飞分隔符""的字符的索引
    std::string::size_type lastPos = s.find_first_not_of(delimiters, 0);

    //从第一个分隔符lastpos开始往后找到第一个分隔符，并返回该分隔符的索引
    std::string::size_type pos = s.find_first_of(delimiters, lastPos);

    //即从上面两个下标来截取一个字符串
    while (std::string::npos != pos || std::string::npos != lastPos) {
        //emplace_back 在vector的结尾插入一个新的元素,相似的方法是push_back
        //substr中第一个参数为：所需的子字符串的起始位置。字符串中第一个字符的索引为 0,默认值为0。
        //substr中第二个参数为：复制的字符数目。
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
    clean(g_listen_fd);  // 标记，线程不继续运行
    exit(0);
}

int main() {
    //即当输入ctrl + c时，将触发信号捕捉，实现线程退出，并打印信息
    signal(SIGINT, sigint); // 捕获ctrl+c信号

    ::srand(time(nullptr)); // 随机数种子
    int random_port = rand() % UINT16_MAX + 3000; // uint16最大值为32767,随机获取1个端口号

    std::cout << "your port is: " << random_port << std::endl;

    // 初始化接收socket,传入IP 和 port 获取文件描述符 g_listen_fd
    if (!init_server_socket(kListenIp, random_port, g_listen_fd)) {
        return 0;
    }

    // 启动一个线程，不停接收消息,并打印远端连接用户的ip 和 port
    std::thread t(recv_thread_proc, g_listen_fd);
    t.detach();

    // 主线程接收用户输入
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
        //切割input中的数据，并将这些数据存入arr数组中，即把ip 和 port 还有信息等存入arr数组中
        split(input, arr, " ");
        if (arr.size() < 2) { //格式为 [IP] [Port] [文本内容]，以空格隔开，回车结束，输入exit，退出程序
            std::cout << "错误的格式" << std::endl;
            continue;
        }

        // 解析端口，正常切割字符串，并存入arr后，arr[1]存的是port ，arr[0]存的是ip
        //atoi 字符串转整数
        int remote_port = atoi(arr[1].c_str());
        if (remote_port <= 0 || remote_port >= UINT16_MAX) { //UINT16_MAX，内置类型，为65535
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
        int ret = ::sendto(g_listen_fd, text.c_str(), text.length(), 0, (struct sockaddr *) &dest_addr,
                           sizeof(dest_addr));
        if (ret == -1) {
            std::cout << "sendto error: " << errno << std::endl;
            break;
        } else {
            std::cout << "already send" << std::endl; // 为什么不能打印success send？udp并不能保证我们的消息对方一定能收到
        }
    }

    clean(g_listen_fd); // 释放所有资源
    std::cout << "exit ..." << std::endl;
    return 0;
}