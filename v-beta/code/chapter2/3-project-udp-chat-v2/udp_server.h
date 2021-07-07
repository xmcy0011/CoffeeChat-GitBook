/** @file udp_server.h
  * @brief 
  * @author teng.qing
  * @date 2021/7/7
  */

#ifndef INC_3_PROJECT_UDP_CHAT_V2_UDP_SERVER_H_
#define INC_3_PROJECT_UDP_CHAT_V2_UDP_SERVER_H_

#include <netinet/in.h> // sockaddr_in
#include <atomic>

/** @class udp_server
  * @brief UDPServer封装
  */
class UdpServer {
public:
    UdpServer(const UdpServer &) = delete; // 拷贝构造

    UdpServer operator=(const UdpServer &) = delete; // 赋值构造

    UdpServer(UdpServer &&) = delete; // C++11移动构造

    /** @fn getInstance
      * @brief 单例，线程安全
      * @return 实例
      */
    static UdpServer *getInstance();

public:
    /** @fn Listen
      * @brief 初始化socket并且绑定IP
      * @param [in]listen_ip: IP
      * @param [in]port: 端口
      * @return 是否成功
      */
    bool listen(const std::string &listen_ip, int port);

    /** @fn Run
      * @brief 启动线程，不停接收数据
      * @return void
      */
    void run();

    /** @fn Stop
      * @brief 停止线程，退出
      * @return void
      */
    void stop();

    /** @fn ListenFd
      * @brief 获取Socket句柄
      * @return int: Socket句柄
      */
    int listenFd() const { return listen_fd_; }

private:
    /** @fn recv_thread_proc
     * @brief 接收数据线程
     * @param [in]listen_fd: 监听socket句柄
     * @return
     */
    void recvThreadProc();

    void onHandle(const char *buffer, int len, struct sockaddr_in &remote_addr);

    UdpServer();

    ~UdpServer();

private:
    int listen_fd_;
    std::atomic_bool recv_thread_run_; // thread run flag
};


#endif //INC_3_PROJECT_UDP_CHAT_V2_UDP_SERVER_H_
