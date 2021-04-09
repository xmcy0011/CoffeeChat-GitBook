# 如何管理大量TCP连接

本章的目标是介绍如何管理大量的TCP连接（UDP要用好真不简单，功力有限，抛开不谈），思路如下：

- 先说挑战，从几个经典问题看起。
- 再说理论，看Linux操作系统几种经典的I/O模型是如何解决这些问题的
- 然后说下代码实现
- 最后会深入探讨下万、十万和百万级别连接管理的架构

## 经典问题：C10K和C10M

### 引言

我印象中小学（2003年）时代一个月才有1次的计算机打字课（进去还要换拖鞋，也是很奇怪），是用的大头机和windows98系统：

![大头机](../images/pc_1990.jpg)

到初中年代（2008），县城里的网吧也还有许多电脑是这种大头机（辐射非常大，玩一会头晕眼花），然后慢慢的才有液晶显示器，直到大学才有自己的第一款笔记本（2010）。

也是从2010年后，自己各种折腾电脑，重装系统，到自己工作，才开始关注CPU，i3、i5和i7，甚至最近2020年新款的mac pro的i9处理器，从单核到双核到4核8线程等等。

计算机飞速发展，现在一台普通的笔记本甚至智能手机都能媲美20世纪的服务器了。【PK图，后续添加】

大学时代，我在i3配置的笔记本上用C#实现了一个windows tcp server，一开始只能支持几百个连接（客户端启动和退出一频繁，服务就崩溃），然后使用了IOCP的技术，实现了几千个连接的支持（受限于笔记本）。直到工作，任意找一台台式机，用Linux写一个epoll服务端，就可以实现上万的连接支持，所以现在C10K（单机支持10*1000个连接）已经不在是问题了。

### C10K

简单来说，C10K就是指单机如何支持1万个（10*1000）连接，主要是在互联网的早期，Web服务器在硬件有限的情况下，如何能支持更高的并发（众所周知HTTP协议是基于TCP之上的，所以需要建立连接）。

虽然目前服务器性能越来越高，但是如果方法不对（Linux下如果使用select I/O复用模型，则默认最大只能支持1024个连接，更多就需要修改内核并重新编译了），也是做不到1万个连接的支持的。

左耳朵耗子大叔的专栏里面有提到，提出这个问题的人叫丹·凯格尔（Dan Kegel），目前在 Google 任职。可以阅读一下这篇文章了解下这个问题：[The C10K problem](http://www.kegel.com/c10k.html)([翻译版](https://www.oschina.net/translate/c10k))

这个问题的本质，引用左耳朵耗子大叔的话来解释：

> C10K 问题本质上是操作系统处理大并发请求的问题。对于 Web 时代的操作系统而言，客户端过来的大量的并发请求，需要创建相应的服务进程或线程。这些进程或线程多了，导致数据拷贝频繁（缓存 I/O、内核将数据拷贝到用户进程空间、阻塞）， 进程 / 线程上下文切换消耗大，从而导致资源被耗尽而崩溃。这就是 C10K 问题的本质。
>
> 了解这个问题，并了解操作系统是如何通过多路复用的技术来解决这个问题的，有助于你了解各种 I/O 和异步模型，这对于你未来的编程和架构能力是相当重要的。

### C10M

更近一步，单机如何支持千万级（10\*1000*1000）连接可以看一下这篇文章：[The Secret To 10 Million Concurrent Connections -The Kernel Is The Problem, Not The Solution](http://highscalability.com/blog/2013/5/13/the-secret-to-10-million-concurrent-connections-the-kernel-i.html)

## 理论基础

### Linux下的IO模型

#### 什么是I/O

首先什么是I/O？

I/O(input/output)，即输入输出设备，现实中键盘和鼠标是输入设备，显示器是输出设备，在《深入理解计算机系统》第一章1.7.4节中，有说到：**文件是对I/O设备的抽象表示，文件就是字节序列，仅此而已。每个I/O设备，包括磁盘、键盘、显示器，甚至网络，都可以看成文件**。

所以，不难理解在Linux的API中，为什么发送TCP数据包可以调用write()，接收数据包可以调用read()了，在《Linux-UNIX系统编程手册》中第56.5.4节流socket I/O中有描述：

![socket-io](../images/socket-io.jpg)

《Netty权威指南 第2版》1.1.1节也有描述：

> Linux的内核将所有外部设备都看作一个文件来操作，对一个文件的读写操作会调用内核提供的系统命令，返回一个file descriptor（fd，文件描述符）。而对一个socket的读写也会有相应的描述符，称为socketfd（socket描述符），描述符就是一个数字，它指向内核中的一个结构体（文件路径，数据去等一些属性）

所以，为什么c++有如下接收数据的代码（第一个参数socket_fd就是socket描述符，它的创建过程这里省略了）：

```c++
while(true){
	// ...
	// 调用read等待网络数据的到来，期间一直阻塞
	size_t len = read(socket_fd, buffer, kMaxBufferLen);
	// 调用write把要发送的数据写入到socket缓冲区，等待发送
	write(socket_fd, buffer, len);
	// ...
}
```


既然网络也被抽象为文件，那么如何使读写（收发）比较快，拥有很高的性能就很关键了，《UNIX网络编程卷1》第6.2节 I/O模型里面介绍了5种模型，我们来一起看看。

#### 5种I/O模型

- 阻塞式I/O
- 非阻塞式I/O
- I/O复用（select和poll）
- 信号驱动式I/O（SIGIO）
- 异步I/O（POSIX的aio_系列函数）

对于Linux C++，后面2种不常用，主要看前3种即可。对于Java，NIO类库封装了所以异步I/O的细节，也只要了解一下操作系统支持异步I/O即可。

##### 阻塞式I/O

![阻塞式I/O](../images/io-model-blocking.png)

最常用最基本的I/O模型，缺省情况下，所有的文件操作都是阻塞的。以UDP为例，在进程空间中调用recvfrom（接收数据），其系统调用直到数据包到达，且被复制到应用进程的缓冲区中或者发送错误时才返回，在此期间一直会等待。进程在从调用recvfrom开始，到它返回的整段时间内都是被阻塞的，因此被称为阻塞I/O模型。

recvfrom的API原型如下：

```c++
/**
 * receive a message from a socket
 * @param s: socket描述符
 * @param buf: UDP数据报缓存区（包含所接收的数据）
 * @param len: 缓冲区长度
 * @param flags: 调用操作方式（一般设置为0）
 * @param from: 指向发送数据的客户端地址信息的结构体（sockaddr_in需类型转换）
 * @param from_len: 指针，指向from结构体长度值
 * @return These calls return the number of bytes received, or -1 if an error occurred
 */
int recvfrom(int socket, void *buf, int len, unsigned int flags,
             struct sockaddr *from, int *from_len);
```

##### 非阻塞式I/O

![非阻塞式I/O](../images/io-model-noblocking.png)

和阻塞式I/O模型的区别，就是调用recvfrom后立即返回，通过返回值判断是否有数据（EWOULDBLOCK错误代表没有数据）。所以一般搭配sleep使用，但是如何确定轮询检查的间隔，看应用场景。

##### I/O复用模型

![I/O复用模型](../images/io-model-multiplexing.png)

这个模型是我们这一章的重点（后面会详细介绍），主要以Linux提供的select/poll/epoll来实现。

- select/poll：进程通过将一个或多个fd传递给select或poll系统调用，阻塞在select操作上，这样select/poll可以帮我们侦测多个fd是否处于就绪状态。select/poll是顺序扫描fd是否就绪，而且支持的fd数量有限（默认1024个，f d受限，也即意味着默认情况下最大只能支持1024个连接），因此它的使用受到了一些制约。

- epoll：使用基于事件驱动的方式代替顺序扫描，因此性能更高。当有fd就绪时，立即进行回调处理。

它的原称是**I/O multiplexing**（有时候“I/O复用”这个翻译，会让人有点疑惑，复用在那里？为什么叫做I/O复用？），即：多路网络连接复用一个io线程的意思，虽然不是很精确，但是能让我们明白为什么叫复用。

再引用来自于 [[知乎：IO 多路复用是什么意思？罗志宇的回答](https://www.zhihu.com/question/32163005)] 一图：

![i/o multiplexing](../images/io multiplexing.gif)

>  **I/O multiplexing 这里面的 multiplexing 指的其实是在单个线程通过记录跟踪每一个Sock(I/O流)的状态(对应空管塔里面的Fight progress strip槽)来同时管理多个I/O流**. 发明它的原因，是尽量多的提高服务器的吞吐能力。
>
> 在同一个线程里面， 通过拨开关的方式，来同时传输多个I/O流， (学过EE的人现在可以站出来义正严辞说这个叫“时分复用”了）。
>
> *什么，你还没有搞懂“一个请求到来了，nginx使用epoll接收请求的过程是怎样的”， 多看看这个图就了解了。提醒下，ngnix会有很多链接进来， epoll会把他们都监视起来，然后像拨开关一样，谁有数据就拨向谁，然后调用相应的代码处理。*

IO复用形成原因具体可以参考这里 [IO复用](https://www.cnblogs.com/nr-zhang/p/10483011.html)：

> 如果一个I/O流进来，我们就开启一个进程处理这个I/O流。那么假设现在有一百万个I/O流进来，那我们就需要开启一百万个进程一一对应处理这些I/O流（——这就是传统意义下的多进程并发处理）。思考一下，一百万个进程，你的CPU占有率会多高，这个实现方式及其的不合理。所以人们提出了I/O多路复用这个模型，一个线程，通过记录I/O流的状态来同时管理多个I/O，可以提高服务器的吞吐能力

##### 信号驱动式I/O（不常用）

![信号驱动式I/O](../images/io-model-signal-driven.png)

信号驱动式I/O是指预先告知内核，使得当某个描述符上发生某事时，内核使用信号通知相关进程。如上图，先开启套接接口信号驱动I/O功能，并通过系统调用sigaction执行一个信号处理函数（此系统调用立即返回，进程继续工作，他是非阻塞的）。当数据准备就绪时，就为该进程生产一个SIGIO信号，通过信号回调通知应用程序调用recvfrom来读取数据，并通知主循环函数处理数据。

##### 异步I/O模型（不常用）

![异步I/O模型](../images/io-model-async.png)

异步I/O：告知内核启动某个操作，并让内核在整个操作完成后（包括将数据从内核复制到用户自己的缓冲区）通知我们。这种模型与信号驱动模型的主要区别是：信号驱动I/O由内核通知我们何时可以开始一个I/O操作，而异步I/O模型由内核通知我们I/O操作何时已经完成。

##### 5种模型对比

![5种模型对比](../images/io-model-compare.png)

### 高性能实现：I/O复用

其实最重要的就是这一节吧，实现大量TCP连接的答案就是epoll技术（Windows下是IOCP技术），但是并不是每种场合下epoll都适用：

- 比如epoll是linux特有的，windows有iocp（完成端口）的技术，所以如果是windows服务器，就不适合。同样macos下是kqueue，原理上和epoll类似。
- epoll适合连接数多，但是都不活跃的场景，比如IM（用户虽然在线，但并不是时时刻刻都在发消息）、消息推送等。select/poll适合对吞吐量要求高，连接数少（千级别）的场景，比如音视频传输（每一秒都在传输大量的数据）、文件传输等，当然对于这些对速度有要求的实时传输业务，可能更好的选择是UDP协议。

下面，让我们通过例子来看一下三种I/O复用模型的差异，同时介绍2种设计模式，让我们更深入的掌握epoll的用法。

#### 架构模型的演变

在正式介绍I/O复用模型前，我们先来一点开胃菜。看看I/O复用出现的背景，以及在没有这种技术之前，前辈门是如何来进行网络服务器编程的。

架构模型的演变涉及面太广太大，这里只罗列2种最常见的场景（UDP和TCP类似），更多的内容读者可自行查阅：
- Web（HTTP）服务器（更多内容：[互联网架构演进模型（一）](https://zhuanlan.zhihu.com/p/162727492)、[互联网架构演进模型（二）](https://zhuanlan.zhihu.com/p/168793240)、[千万级并发下，淘宝服务端架构如何演进？](https://developer.51cto.com/art/201906/597895.htm)）
	1. 单机网站架构
	2. 应用与数据分离
	3. 使用缓存改善网站性能
	4. 应用服务器集群
	5. 数据库读写分离
	6. 使用反向代理和CDN加速网站响应
	7. 数据库的分库分表（垂直/水平拆分）及分布式文件系统
	8. 使用NoSQL和搜索引擎
	9. 按业务模块拆分
	10. 服务化及中间件
	11. 微服务和分布式（上面的文章没有提到）
	12. 云（SAAS、PAAS）（上面的文章没有提到）
- 游戏（TCP）服务器（更多内容：[游戏服务器的架构演进(完整版)](https://www.gameres.com/799041.html)）
	1. 第一代网游服务器（单线程无阻塞）
	2. 第二代网游服务器（分区分服）
	3. 第三代网游服务器（三层架构 -> cluster -> 无缝地图）

从上面2种经典的服务演进可以看出，都是由单机版进化而来，这里为了简单起见，我们只从最基础的单机应用的视角来介绍，下面我们一起来看2个具体的例子。

##### 单线程+阻塞I/O

我们先来看一个TCP服务器最基础的实现（基于单进程+单线程方式），以下是一个示例：  
server.cpp
```c++
#include <iostream>

#include <cstring>
#include <cerrno>
#include <netinet/in.h> // ipv4: PF_INET,sockaddr_in ,v6:PF_INET6,sockaddr_in6
#include <sys/socket.h> // socket,bind,listen,accept
#include <unistd.h>     // read,close
#include <arpa/inet.h> // inet_addr

const int kSocketError = -1;

/** @fn main
  * @brief 演示socket的基础调用demo，使用了默认同步I/O阻塞+单线程的方式，即同时只能处理1个连接，直到这个连接断开后才能处理下一个连接。
  * @return
  */
int main() {
    // 创建监听socket的实例，返回socket文件句柄
    int listenFd = ::socket(PF_INET, SOCK_STREAM, 0);
    if (listenFd == kSocketError) {
        std::cout << "create socket error:" << errno << std::endl;
        return 0;
    }
    std::cout << "create socket" << std::endl;

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8088);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // 绑定到127.0.0.1:8088的本地回环地址，客户端连接时的服务器IP和端口要保持和这里一致
    int ret = ::bind(listenFd, (sockaddr *) &addr, sizeof(addr));
    if (ret == kSocketError) {
        std::cout << "bind socket error:" << errno << std::endl;
        return 0;
    }

    std::cout << "bind success,start listen..." << std::endl;
    // 标识文件描述符为被动socket，这个socket会被用来接收来自其他（主动）socket的连接
    ret = ::listen(listenFd, SOMAXCONN);
    if (ret == kSocketError) {
        std::cout << "listen error:" << errno << std::endl;
        return 0;
    }

    // 服务要不停的执行，本质上就是个死循环
    while (true) {
        struct sockaddr_in peerAddr{};
        socklen_t sockLen = sizeof(sockaddr_in);
        // accept()系统调用在文件描述符 sockfd 引用的监听流 socket 上接受一个接入连接
        // 如果没有未决的连接，会一直阻塞直到有连接请求到达为止。
        int fd = ::accept(listenFd, (sockaddr *) &peerAddr, &sockLen);
        if (fd == kSocketError) {
            return 0;
        }
        std::cout << "new connect coming,accept..." << std::endl;
        while (true) {
            char buffer[1024] = {};
            // 接收来自于客户端的数据，如果没有数据到来，则一直阻塞
            ssize_t len = recv(fd, buffer, sizeof(buffer), 0); // wait
            if (len == kSocketError) {
                std::cout << "recv error:" << errno << std::endl;
                break;

            } else if (len == 0) {
                std::cout << "recv error:" << errno << std::endl;
                break;

            } else {
                std::cout << "recv: " << buffer << ",len=" << len << std::endl;
                // echo，给客户端回复相同的内容，就像回声一样
                len = send(fd, buffer, len, 0);
                if (len == kSocketError) {
                    std::cout << "send error:" << errno << std::endl;
                    break;
                }
            }
        }
        ::close(fd);
        std::cout << "remote " << ::inet_ntoa(peerAddr.sin_addr) << "close connection" << std::endl;
    }

    return 0;
}
```

client.cpp
```c++
#include <iostream>

#include <cerrno>
#include <thread>
#include <sys/socket.h> // bind,connect
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h>  // inet_addr()
#include <unistd.h>     // close

const int kSocketError = -1;

int main() {
	// 创建实例
    int fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == kSocketError) {
        std::cout << "socket error:" << errno << std::endl;
        return 0;
    }

    struct sockaddr_in serverIp{};
    serverIp.sin_family = AF_INET;
    serverIp.sin_port = htons(8088);
    serverIp.sin_addr.s_addr = inet_addr("127.0.0.1");

    // 连接远端tcp server
    std::cout << "connect remote" << std::endl;
    int ret = ::connect(fd, (sockaddr *) &serverIp, sizeof(serverIp));
    if (ret == kSocketError) {
        std::cout << "connect error:" << errno << std::endl;
        return 0;
    }

    char buffer[1024] = {0};
    char recvBuffer[1024] = {0};
    for (int i = 0; i < 10; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        int len = sprintf(buffer, "hello %d", i);
        // 发送
        ret = ::send(fd, buffer, len, 0);
        if (ret == kSocketError) {
            std::cout << "send error:" << errno << std::endl;
            break;
        }

        // 接收应答，如果服务没有相应，则阻塞
        ret = ::recv(fd, recvBuffer, sizeof(recvBuffer), 0);
        if (ret == kSocketError) {
            std::cout << "send error:" << errno << std::endl;
            break;
        }
        std::cout << "recv from:" << recvBuffer << std::endl;
    }
    ::close(fd);

    return 0;
}
```

这个例子有什么问题？我们使用g++编译后，实际运行，当启动第二个client程序的时候，我们会发现 **connect()** 连不上去，为什么？

来分析一下这段代码：
```c++
// 死循环，持续监听，只要有新连接到来，accept就会返回一个与客户端建立了连接的socket fd，此时就可以通信了
while (true) {
	// ...
	// 没有新的连接到来accept会一直阻塞
	int fd = ::accept(listenFd, (sockaddr *) &peerAddr, &sockLen);
	// ...
	while (true) {
		char buffer[1024] = {};
		// recv会一直阻塞（可以设置O_NONBLOCK变成非阻塞I/O，但默认是阻塞的），直到：
		// 1. 客户端发送数据
		// 2. 客户端调用close()断开连接
		ssize_t len = recv(fd, buffer, sizeof(buffer), 0); 
		// ..
	}
	// ...
}
```

这里有2个死循环，最外层的循环用以监听新的客户端连接的到来，里面的循环则通过 **recv()** 系统调用来接收客户端发来的数据，直到客户端关闭，**recv()** 返回0，此时循环退出。执行到 **accept()** 开始接受下一个客户端的连接请求。

我们发现，这其实是一个串行的过程，即同一时间，只能接收1个客户端连接，其他的连接都得排队，这明显不合理，C/S架构变成了P2P架构。通常改进的方式有2种：
1. 使用非阻塞I/O+轮询
2. 使用多线程+阻塞I/O

第一种方式，需要至少2个线程配合，1个线程调用 **accept()** 处理新连接并且把socket fd加入到一个全局链表，另1个线程则从这个链表种每隔一定时间就进行o(n)级别的遍历，对每一个socket fd调用 **recv()** 来检查客户端是否发来数据，然后处理。

第二种方式，也是我们接下来会介绍的方式，相比第一种，它的优点是简单，及时性好。缺点是一个连接一个线程，内存占用会更多，且操作系统对线程数量有所限制。

##### 多线程+阻塞I/O

为了解决单进程单线程下只能同时处理一个连接的问题，我们引入多线程技术，核心思路是把 **recv()** 放在单独的线程中执行，这样最外层的while循环得以执行到 **accept()** ，即意味着可接收新的连接的到来，真正的实现了服务器的功能，可以处理很多个TCP客户端。

![io-multiplexing-thread-compare](../images/chapter2/io-multiplexing-thread-compare.png)

下面是一个实例（注意，其他部分没变，只是在上一节中while循环的代码中，引入了线程而已）：

```c++
// 死循环，永不退出
while (true) {
	struct sockaddr_in peerAddr{};
	socklen_t sockLen = sizeof(sockaddr_in);
	// will sleep, until one connection coming
	int fd = ::accept(listenFd, (sockaddr *) &peerAddr, &sockLen);
	if (fd == kSocketError) {
		return 0;
	}

	std::cout << "new connect coming,accept..." << std::endl;

	// 改进：lamda表达式，这是一个匿名函数
	auto proc = [fd, peerAddr]() {
		while (true) {
			char buffer[1024] = {};
			// 没有数据时会阻塞
			ssize_t len = recv(fd, buffer, sizeof(buffer), 0); // wait
			if (len == kSocketError) {
				std::cout << "recv error:" << errno << std::endl;
				break;

			} else if (len == 0) {// 客户端端口连接了，直接结束
				std::cout << "recv error:" << errno << std::endl;
				break;

			} else {
				std::cout << "recv" << ::inet_ntoa(peerAddr.sin_addr) << ":" << peerAddr.sin_port
							<< " " << buffer << ",len=" << len << std::endl;
				// echo
				len = send(fd, buffer, len, 0);
				if (len == kSocketError) {
					std::cout << "send error:" << errno << std::endl;
					break;
				}
			}
		}

		// tcp是全双工通信，我们也需要主动关闭，否则会耗尽系统的文件句柄资源
		::close(fd);
		std::cout << "remote " << ::inet_ntoa(peerAddr.sin_addr) << "close connection" << std::endl;
	}; // end lamda

    // 启动c++11标准线程库，可跨平台，头文件是<thread>
	// proc是一个匿名函数，线程构造完成，自动启动后就会回调执行
	std::thread thread(proc);
	// 分离线程，主线程结束时继续存在；线程结束后，立马回收资源。
	thread.detach();

	// 继续往下执行，也就是下一个while的循环，即执行到::accept()部分后阻塞，
	// 直到下一个连接到来
}
```

上面的代码，通过**在不同的线程中处理 accept() 和 recv() 2个会导致阻塞的调用**，这样就拥有了并行的能力，可以同时处理新连接和客户端发来的数据了。PS：main()函数本身就被一个线程调用，我们称之为主线程。

但是，线程数量在Linux中是有限制的，在CentOS下：
```bash
$ cat /proc/sys/kernel/thread-max 
7765
```

另外，每启动一个线程，都会付出额外的内存代价：
```bash
$ ulimit -s
8192
```

下面有一组数据：
```txt
线程数  VIRT(虚拟内存)  RES(物理内存)
1      14696(14MB)     852(0.8MB)
11     96788(94MB)     1100(1MB)
101    834428(814MB)   1628(1.5MB)
```

关于VIRT和RES的区别，可以参考这里：[linux top命令 实存(RES)与虚存(VIRT)详解](https://blog.csdn.net/liu_sisi/article/details/88633673)
> 堆、栈分配的内存，如果没有使用是不会占用实存的，只会记录到虚存。
> 如果程序占用实存比较多，说明程序申请内存多，实际使用的空间也多。
> 如果程序占用虚存比较多，说明程序申请来很多空间，但是没有使用。

因为线程数量的限制和创建线程代价很大的问题（尽管可以使用线程池一定程度解决，但因为线程数量的限制还是无法实现单机1万个连接以上），实际中很少会这样来实现。

##### 非阻塞I/O

在游戏服务器的早期，使用过这种方式，因为不是我们的重点，所以我们这里就不再详细描述。这种单进程+非阻塞I/O可以配合多线程来实现更高的性能，但是实现复杂，要考虑多线程面临的各种问题（死锁、同步），所以不推荐。

#### 三种I/O复用模型

下面来看一下，操作系统层面给我们提供的解决方案。

我们来回顾一下这三种模型：
- select/poll：进程通过将一个或多个fd传递给select或poll系统调用，阻塞在 **select()** 操作上，这样select/poll可以帮我们侦测多个fd是否处于就绪状态。select/poll是顺序扫描fd是否就绪，其中select支持的fd数量有限（最大默认支持1024个连接），但是在各个操作系统上都支持，也因此它的使用受到了一些制约。而poll没有最大数量的限制，但是随着连接数的增多，性能会直线下降（**o(n)时间复杂度**）。
- epoll：使用基于事件驱动的方式代替顺序扫描，因此性能更高（**o(1)时间复杂度**）。当有fd就绪时，立即进行回调处理。

下面看一下具体的代码。

##### select/poll


因为poll和select类似，这里的代码就以 select 为例来说明：
```c++
#include <iostream>

#include <cstring>
#include <cerrno>
#include <thread>       // thread
#include <netinet/in.h> // ipv4: PF_INET,sockaddr_in ,v6:PF_INET6,sockaddr_in6
#include <sys/socket.h> // socket(),bind(),listen(),accept()
#include <unistd.h>     // read(),close()
#include <arpa/inet.h>  // inet_addr()
#include <fcntl.h>      // fcntl()

#include <sys/select.h> // select()

#include <vector>  // vector

const int kSocketError = -1;

/** @fn main
  * @brief 演示socket的基础调用demo，使用了select I/O复用模型，主要包括select、
  * FD_CLR()、FD_ISSET()、FD_SET()、FD_ZERO()等几个函数。
  * 优点：
  *     1. 相比多线程方案，这里所有的处理都在1个线程上，不用考虑线程并发带来的死锁和同步等问题，内存占用更低，开发更简单。
  *     2. 相比epoll和poll，移植性更好，在windows\mac等平台都支持。
  * 缺点：
  *     1. o(n)级别顺序扫描，效率较低；
  *     2. select最大只支持1024个连接，而poll没有这个限制，但是o(n)级别的扫描，链接数越多，性能则越低。
  *
  * @return
  */
int main() {
    int listenFd = 0;
    int ret = 0;
    int yesReuseAddr = 1;
    struct sockaddr_in addr{};
    std::vector<int> connections; // 已建立的连接
    struct timeval tv{};                // select超时
    fd_set readFds;                     // select 可读fd集合
    int maxFd = 0;                      // select 最大的fd
    char recvBuffer[1024] = {0};        // 接收缓冲区

    listenFd = ::socket(PF_INET, SOCK_STREAM, 0);
    if (listenFd == kSocketError) {
        std::cout << "create socket error:" << errno << std::endl;
        return 0;
    }
    std::cout << "create socket" << std::endl;

    // non-block
    ret = ::fcntl(listenFd, F_SETFL, O_NONBLOCK);
    if (ret == kSocketError) {
        std::cout << "fcntl error:" << errno << std::endl;
        return 0;
    }

    // SO_REUSEADDR
    if (::setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &yesReuseAddr, sizeof(yesReuseAddr)) == kSocketError) {
        std::cout << "setsockopt error:" << errno << std::endl;
        return 0;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(8088);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    ret = ::bind(listenFd, (sockaddr *) &addr, sizeof(addr));
    if (ret == kSocketError) {
        std::cout << "bind socket error:" << errno << std::endl;
        return 0;
    }

    std::cout << "bind success,start listen..." << std::endl;
    // 标识文件描述符为被动socket
    ret = ::listen(listenFd, SOMAXCONN);
    if (ret == kSocketError) {
        std::cout << "listen error:" << errno << std::endl;
        return 0;
    }

    for (;;) {
        tv.tv_sec = 30;
        tv.tv_usec = 0;

        // select函数会清空各个集合，每次都需要加入
        FD_ZERO(&readFds);
        // Listen的socket加入监听
        FD_SET(listenFd, &readFds);
        maxFd = listenFd;
        // 已连接的socket加入监听
        for (const auto &sockFd : connections) {
            FD_SET(sockFd, &readFds);
            if (sockFd > maxFd) {
                maxFd = sockFd;
            }
        }

        ret = ::select(maxFd + 1, &readFds, nullptr, nullptr, &tv);
        if (ret == -1) {
            std::cout << "select error:" << errno << std::endl;
            continue;
        } else if (ret == 0) {
            std::cout << "select timeout:" << errno << std::endl;
            continue;
        }

        if (FD_ISSET(listenFd, &readFds)) {
            int clientFd = ::accept(listenFd, nullptr, nullptr);
            if (clientFd == kSocketError) {
                std::cout << "accept error:" << clientFd << std::endl;
                continue;
            }
            std::cout << "new connect coming" << std::endl;
            connections.push_back(clientFd);
            if (clientFd > maxFd) { // update
                std::cout << "update maxSock ,before:" << maxFd << ",now:" << clientFd << std::endl;
                maxFd = clientFd;
            }
            FD_SET(clientFd, &readFds); // 加入到监听列表
        } else {
            // select的缺点，每一次都需要o(n)级别的遍历
            for (auto it = connections.begin(); it != connections.end();) {
                int sockFd = *it;
                if (!FD_ISSET(sockFd, &readFds)) { // 没有数据，直接下一个
                    ++it;
                    continue;
                }

                // 获取客户端IP
                struct sockaddr_in peer{};
                socklen_t len = sizeof(sockaddr_in);
                if (::getpeername(sockFd, (sockaddr *) &peer, &len) == kSocketError) {
                    std::cout << "getpeername error" << errno << std::endl;
                    ++it;
                    continue;
                }

                // 因为上面通过FD_ISSET()检查了有数据，所以不会阻塞。
                // 但通常情况下，应该在accept()返回sockfd之后，行设置O_NONBLOCK显示声明为非阻塞I
                ret = ::recv(sockFd, recvBuffer, sizeof(recvBuffer), 0);
                if (ret == 0) { // EOF
                    std::cout << "client " << sockFd << " close ths connection, " << inet_ntoa(peer.sin_addr) << ":"
                              << ::ntohs(peer.sin_port) << std::endl;
                    FD_CLR(sockFd, &readFds);
                    ::close(sockFd);
                    it = connections.erase(it);

                } else if (ret == -1) {
                    std::cout << "recv error:" << errno << std::endl;
                    FD_CLR(sockFd, &readFds);
                    ::close(sockFd);
                    it = connections.erase(it);

                } else {
                    std::cout << "recv from " << inet_ntoa(peer.sin_addr) << ":" << ::ntohs(peer.sin_port)
                              << ": " << recvBuffer << ",len=" << ret << std::endl;
                    // echo
                    ::send(sockFd, recvBuffer, ret, 0);
                    ++it;
                }
            }
        }
    }

    return 0;
}
```

[代码解读视频：Bilibili](待补充)

 **select()** 和 **poll()** 存在的问题（《Linux-UNIX系统编程手册 63.2.5章》）：
- 每次调用 select()或 poll()，内核都必须检查所有被指定的文件描述符，看它们是否处
于就绪态。当检查大量处于密集范围内的文件描述符时，该操作耗费的时间将大大超过接下来的操作。
- 每次调用 select()或 poll()时，**程序都必须传递一个表示所有需要被检查的文件描述符
的数据结构到内核**，内核检查过描述符后，修改这个数据结构并返回给程序。（此外，
对于 select()来说，我们还必须在每次调用前初始化这个数据结构。）对于 poll()来说，
随着待检查的文件描述符数量的增加，传递给内核的数据结构大小也会随之增加。当检
查大量文件描述符时，**从用户空间到内核空间来回拷贝这个数据结构将占用大量
的 CPU 时间**。对于 select()来说，这个数据结构的大小固定为 FD_ SETSIZE，与待检
查的文件描述符数量无关。
- select()或 poll()调用完成后，程序必须检查返回的数据结构中的每个元素，以此查明
哪个文件描述符处于就绪态了。

上述要点产生的结果就是随着待检查的文件描述符数量的增加，select()和 poll()所占用的
CPU 时间也会随之增加（更多细节请参见 63.4.5 节）。对于需要检查大量文件描述符的程序来
说，这就产生了问题。

我们接下来要讨论的信号驱动 I/O 以及 epoll 都可以使内核记录下进程中感兴趣的文件描
述符，通过这种机制消除了 select()和 poll()的性能延展问题。**这种解决方案可根据发生的 I/O事件来延展，而与被检查的文件描述符个数无关**。结果就是，当需要检查大量的文件描述符
时，信号驱动 I/O 和 epoll 能提供更好的性能表现。

epoll API 的主要优点如下。
- **当检查大量的文件描述符时，epoll 的性能延展性比 select()和 poll()高很多**。
- epoll API **既支持水平触发也支持边缘触发**。与之相反，select()和 poll()只支持水平触
发，而信号驱动 I/O 只支持边缘触发。性能表现上，epoll 同信号驱动 I/O 相似。但是，epoll 有一些胜过信号驱动 I/O 的优点。
- 可以避免复杂的信号处理流程（比如信号队列溢出时的处理）。 
- 灵活性高，可以指定我们希望检查的事件类型（例如，检查套接字文件描述符的读就绪、写就绪或者两者同时指定）。



下面再来看一下epoll的API如何使用。

##### epoll

epoll主要包括 **epoll_create()**、**epoll_ctl()**、**epoll_wait()** 3个函数，使用起来，甚至比select更简单。

server.cpp
```c++
#include <iostream>

#include <cerrno>
#include <netinet/in.h> // ipv4: PF_INET,sockaddr_in ,v6:PF_INET6,sockaddr_in6
#include <sys/socket.h> // socket(),bind(),listen(),accept()
#include <unistd.h>     // read(),close()
#include <arpa/inet.h>  // inet_addr()
#include <fcntl.h>      // fcntl()

#include <sys/epoll.h>  // epoll
#include <vector>       // vector
#include <cstring>

const int kSocketError = -1;

#define USE_ET  // 启用边缘触发

void onNewConnect(const int &epFd, const int &listenFd) {
    int clientFd = ::accept(listenFd, nullptr, nullptr);
    if (clientFd == kSocketError) {
        std::cout << "accept error:" << clientFd << std::endl;
        return;
    }

    // 加入到兴趣列表
    struct epoll_event ev{};
    ev.events = EPOLLIN;

    // 边缘触发模式，追加标志。使用'或'(|)拼接
#ifdef USE_ET
    ev.events |= EPOLLET;
    // 边缘触发模式，只支持非阻塞IO non-block
    int ret = ::fcntl(clientFd, F_SETFL, O_NONBLOCK);
    if (ret == kSocketError) {
        std::cout << "fcntl error:" << errno << std::endl;
        return;
    }
#endif

    ev.data.fd = clientFd;
    if (::epoll_ctl(epFd, EPOLL_CTL_ADD, clientFd, &ev) == kSocketError) {
        std::cout << "epoll_ctl error:" << errno << std::endl;
        ::close(clientFd);
    }

    if (ev.events & EPOLLET) {
        std::cout << "new connect coming,set EPOLLET" << std::endl;
    } else {
        std::cout << "new connect coming" << std::endl;
    }
}

#ifdef USE_ET

void onRead(const int &epFd, const int &fd, const int &listenFd, char recvBuffer[], int bufferLen) {
    // 获取客户端IP
    struct sockaddr_in peer{};
    socklen_t len = sizeof(sockaddr_in);
    if (::getpeername(fd, (sockaddr *) &peer, &len) == kSocketError) {
        std::cout << "getpeername error" << strerror(errno) << std::endl;
        ::close(fd);
        return;
    }

    // 边缘触发模式下，要一次性从socket的recv缓冲区中把数据读出来，
    // 否则下一次不再触发读事件，导致数据丢失
    // 注意：这里就会导致所谓的TCP粘包问题，应用层协议自己通过TLV处理，这里只是为了方便演示故简化
    char *recvBufferStart = recvBuffer;
    int ret = 0;
    int recvTotalLen = 0;
    while (true) {
        ret = ::recv(fd, recvBufferStart, bufferLen - recvTotalLen, 0);
        if (ret == 0) { // EOF，0代表对端断开连接，需要关闭socket
            std::cout << "EOF: client " << fd << " close ths connection, " << inet_ntoa(peer.sin_addr) << ":"
                      << ::ntohs(peer.sin_port) << std::endl;
            // 移除
            if (::epoll_ctl(epFd, EPOLL_CTL_DEL, fd, nullptr) == kSocketError) {
                std::cout << "epoll_ctl error:" << errno << std::endl;
            }
            ::close(fd);
            break;
        } else if (ret < 0) {
            break;
        }
        recvTotalLen += ret;
        recvBufferStart += ret;
    }

    if (ret == -1 && recvTotalLen == 0) {
        std::cout << "recv error:" << errno << std::endl;
        if (::epoll_ctl(epFd, EPOLL_CTL_DEL, fd, nullptr) == kSocketError) {
            std::cout << "epoll_ctl error:" << errno << std::endl;
        }
        ::close(fd);

    } else {
        std::cout << "recv from " << inet_ntoa(peer.sin_addr) << ":" << ::ntohs(peer.sin_port)
                  << ": " << std::string(recvBuffer, recvTotalLen) << ",len=" << recvTotalLen << std::endl;
        // echo
        ::send(fd, recvBuffer, recvTotalLen, 0);
    }
}

#elif
void onRead(const int &epFd, const int &fd, const int &listenFd, char recvBuffer[], int bufferLen) {
    // 获取客户端IP
    struct sockaddr_in peer{};
    socklen_t len = sizeof(sockaddr_in);
    if (::getpeername(fd, (sockaddr *) &peer, &len) == kSocketError) {
        std::cout << "getpeername error" << errno << std::endl;
        return;
    }

    int ret = ::recv(fd, recvBuffer, bufferLen, 0);
    if (ret == 0) { // EOF
        std::cout << "EOF: client " << fd << " close ths connection, " << inet_ntoa(peer.sin_addr) << ":"
                  << ::ntohs(peer.sin_port) << std::endl;
        // 移除
        if (::epoll_ctl(epFd, EPOLL_CTL_DEL, fd, nullptr) == kSocketError) {
            std::cout << "epoll_ctl error:" << errno << std::endl;
        }
        ::close(fd);

    } else if (ret == -1) {
        std::cout << "recv error:" << errno << std::endl;
        if (::epoll_ctl(epFd, EPOLL_CTL_DEL, fd, nullptr) == kSocketError) {
            std::cout << "epoll_ctl error:" << errno << std::endl;
        }
        ::close(fd);

    } else {
        std::cout << "recv from " << inet_ntoa(peer.sin_addr) << ":" << ::ntohs(peer.sin_port)
                  << ": " << recvBuffer << ",len=" << ret << std::endl;
        // echo
        ::send(fd, recvBuffer, ret, 0);
    }
}
#endif

void onClose(const int &epFd, const int &fd) {
    std::cout << "remove close connection" << std::endl;

    if (::epoll_ctl(epFd, EPOLL_CTL_DEL, fd, nullptr) == kSocketError) {
        std::cout << "epoll_ctl error:" << errno << std::endl;
    }
}

bool setNoBlock(const int &fd) {
    // non-block
    int ret = ::fcntl(fd, F_SETFL, O_NONBLOCK);
    if (ret == kSocketError) {
        std::cout << "fcntl error:" << errno << std::endl;
    }
    return ret != kSocketError;
}

/** @fn main
  * @brief 演示socket的基础调用demo，使用了epoll I/O复用模型。
  * 优点：
  *     1. 相比select/poll，改进了扫描算法(o(1))，常量时间复杂度使性能和连接数无关，做到万甚至10万级别的连接。
  *     2. 相比select，没有最大数量限制。只取决于操作系统最大可打开文件的句柄数(可更改，通过ulimit -a查看)
  * 缺点：
  *     1. Linux特有，无法移植
  *     2. 只适用于连接数很多，但是不怎么活跃的场景。如果每个连接都很活跃（参考视频直播，无时无刻都在传输数据），性能退化，甚至不如poll
  * @return
  */
int main() {
    int listenFd = 0;
    int ret = 0;
    int yesReuseAddr = 1;
    struct sockaddr_in addr{};
    int epFd = 0;                       // epoll实例
    int maxFiles = 1024;                // 最大可打开文件的数量
    int fdNum = 0;                      // epoll_wait返回产生事件的描述法数量
#ifdef USE_ET
    char recvBuffer[10 * 1024] = {0};   // 接收缓冲区，边缘触发模式下要一次性读完，所以缓冲区要大一些
#elif
    char recvBuffer[1024] = {0};        // 接收缓冲区
#endif

    // get max open file
    maxFiles = static_cast<int>(::sysconf(_SC_OPEN_MAX));
    if (maxFiles == kSocketError) {
        return 0;
    }
    std::cout << maxFiles << " max file can open" << std::endl;

    // create socket
    listenFd = ::socket(PF_INET, SOCK_STREAM, 0);
    if (listenFd == kSocketError) {
        std::cout << "create socket error:" << errno << std::endl;
        return 0;
    }
    std::cout << "create socket" << std::endl;

    // non-block
    if (!setNoBlock(listenFd)) {
        return 0;
    }

    // SO_REUSEADDR
    if (::setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &yesReuseAddr, sizeof(yesReuseAddr)) == kSocketError) {
        std::cout << "setsockopt error:" << errno << std::endl;
        return 0;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(8088);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    ret = ::bind(listenFd, (sockaddr *) &addr, sizeof(addr));
    if (ret == kSocketError) {
        std::cout << "bind socket error:" << errno << std::endl;
        return 0;
    }

    std::cout << "bind success,start listen..." << std::endl;
    // 标识文件描述符为被动socket
    ret = ::listen(listenFd, SOMAXCONN);//SOMAXCONN in ubuntu 4096,in centos7,128
    if (ret == kSocketError) {
        std::cout << "listen error:" << errno << std::endl;
        return 0;
    }

    maxFiles /= 2;
    epFd = ::epoll_create(maxFiles); // 只是告诉内核一个大致数目，不是最大数量
    if (epFd == kSocketError) {
        std::cout << "epoll_create error" << errno << std::endl;
        return 0;
    }

    // listen的socket加入到epoll监听列表
    struct epoll_event ev{};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = listenFd;
    ret = ::epoll_ctl(epFd, EPOLL_CTL_ADD, listenFd, &ev);
    if (ret == kSocketError) {
        std::cout << "epoll_ctl error:" << errno << std::endl;
        return 0;
    }

    struct epoll_event events[maxFiles];
    for (;;) {
        int timeout = 10 * 1000; // actually 200 ms
         // o(1) 时间复杂度，epoll会直接告诉我们那些socket可读可写而不需要我们自己去遍历
        // 这里会阻塞，超时或者某个socket fd上有可读可写事件时，继续
        fdNum = ::epoll_wait(epFd, events, maxFiles, timeout);

        // 这是select模型的写法，o(n)时间复杂度，需要我们自己遍历，去找那个socket fd上可读
        //for (auto it = connections.begin(); it != connections.end();) {
        //    int sockFd = *it;
        //    if (!FD_ISSET(sockFd, &readFds))

        if (fdNum == -1) {
            std::cout << "epoll_wait error:" << errno << std::endl;
            continue;
        } else if (fdNum == 0) {
            std::cout << "epoll_wait timeout:" << errno << std::endl;
            continue;
        }

        for (int i = 0; i < fdNum; ++i) {
            const epoll_event &e = events[i];
            int fd = e.data.fd;

            if (e.events & EPOLLIN) {
                if (fd == listenFd) {// 监听socket上读事件，代表新的连接到来
                    onNewConnect(epFd, listenFd);
                } else {
                    onRead(epFd, fd, listenFd, recvBuffer, sizeof(recvBuffer));
                }
            }
#ifdef EPOLLRDHUP
            else if (e.events & EPOLLRDHUP) { // linux 2.6.17 以上
                onClose(epFd, fd);
            }
#endif
            else if (e.events & (EPOLLHUP | EPOLLERR | EPOLLPRI)) { // 挂断、错误、收到高优先级数据都需要关闭连接
                onClose(epFd, fd);
            }
        }

    }

    return 0;
}
```

我们可以通过对 **单线程+阻塞I/O下面的client.cpp** 进行改造，来测试该模型下能支持多少个连接。

client.cpp
```c++
#include <iostream>

#include <cerrno>
#include <thread>
#include <sys/socket.h> // bind,connect
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h>  // inet_addr()
#include <unistd.h>     // close
#include <functional>
#include <cstring>

const int kSocketError = -1;

void robot(int id) {
    int fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == kSocketError) {
        std::cout << "socket error:" << errno << std::endl;
        return;
    }

    struct sockaddr_in serverIp{};
    serverIp.sin_family = AF_INET;
    serverIp.sin_port = htons(8088);
    serverIp.sin_addr.s_addr = inet_addr("127.0.0.1");

    int ret = ::connect(fd, (sockaddr *) &serverIp, sizeof(serverIp));
    if (ret == kSocketError) {
        std::cout << "connect error:" << strerror(errno) << std::endl;
        return;
    }
    std::cout << id << " connect remote success" << std::endl;

    // recv 5秒超时
    struct timeval timeout = {5, 0};
    // send 超时 这样设置
    //ret = ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, sizeof(timeout));
    ret = ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout));
    if (ret == kSocketError) {
        std::cout << "setsockopt err" << errno << std::endl;
        return;
    }

    char buffer[1024] = {0};
    char recvBuffer[1024] = {0};
    for (int i = 0; i < 10; ++i) {
        long sleep = ::random() % 3000 + 3000;
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep));
        int len = sprintf(buffer, "hello %d", i);

        std::cout << "send " << std::endl;
        ret = ::send(fd, buffer, len, 0);
        if (ret == kSocketError) {
            std::cout << "send error:" << errno << std::endl;
            break;
        }

        ret = ::recv(fd, recvBuffer, sizeof(recvBuffer), 0);
        if (ret == kSocketError) {
            std::cout << "recv error:" << errno << std::endl;
            continue;
        }
        std::cout << "recv from:" << recvBuffer << std::endl;
    }
    ::close(fd);
    std::cout << "close " << id << std::endl;
}

int main() {
    ::srandom(time(nullptr));

    const int clientNum = 1000;
    for (int i = 0; i < clientNum; ++i) {
        std::thread t(robot, i);
        t.detach();

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // wait client exit
    std::this_thread::sleep_for(std::chrono::seconds(60));
    return 0;
}
```

所有代码在这里：[github](https://github.com/xmcy0011/geeker-skills/tree/master/code/tcp)

[代码解读视频：Bilibili](待补充)

至此，三种I/O模型介绍完毕，下面再补充介绍一下epoll的2种模式：
- LT：水平触发（默认），epoll 会告诉我们何时能在文件描述符上以非阻塞的方式执行 I/O 操作，**对于存在未读完的数据，下一次调用epoll_wait时还会触发**。
    - 优点：简单，易于编码，未读完的数据下次还能继续读，不易遗漏
    - 缺点：在并发量高的时候，epoll_wait返回的就绪队列比较大，遍历比较耗时。因此LT适用于并发量小的情况
- ET：边缘触发，和LT相比，**无论是否读完，只触发一次，直到下一次EPOLLIN事件到来**。ET模式在很大程度上减少了epoll事件被重复触发的次数，因此效率要比LT模式高。只支持非阻塞I/O，以避免由于一个文件句柄的阻塞读/阻塞写操作把处理多个文件描述符的任务饿死。
    - 优点：并发量大的时候，就绪队列要比LT小得多，效率更高
    - 缺点：复杂，难以编码，需要一次读完，有时会遗漏；可能会遇到文件描述符饥饿问题（有1个描述符上有大量的输入，使其他描述符处于饥饿状态），轮转调度（round-robin）方式循环处理带来了更高的复杂度。

参考：
- [Linux I/O复用中select poll epoll模型的介绍及其优缺点的比较](https://zhuanlan.zhihu.com/p/141447239)
- [Linux IO模式及 select、poll、epoll详解](https://segmentfault.com/a/1190000003063859#articleHeader0)
- [EPOLL中LT和ET优缺点](https://blog.csdn.net/a13602955218/article/details/105325146)


##### 三种I/O模型比较

![select-poll-epoll-compare.png](../images/chapter2/select-poll-epoll-compare.png)[来源](https://zhuanlan.zhihu.com/p/141447239)

#### 2种设计模式

在初步掌握了epoll的api之后，我们需要更进一步探讨一下实际的环境中是如何使用epoll的。

因为I/O Multiplexing技术的出现，目前市面上高性能的主流开源网络库使用的事件驱动模型主要分成2大阵营：
- Reactor模型（反应器）：以linux(epoll)、darwin/macos(kqueue)2个平台为主，在Linux下开发网络服务器程序的主流模型。使用这种模型的有：
    - [muduo](https://github.com/chenshuo/muduo), [evpp](https://github.com/Qihoo360/evpp)
    - [libevent](https://github.com/libevent/libevent), libev
    - ACE, [Poco C++ Libraries](https://github.com/pocoproject/poco)
    - Java NIO，包括 [Apache Mina](https://github.com/mina-deploy/mina) 和 [Netty](https://github.com/netty/netty) 
- Preactor模型（主动器）：epoll由于是linux特有，故windows下对应的是iocp（Input/Output Completion Port）技术，该模型通常在windows上使用。不过跨平台的 [asio](https://github.com/chriskohlhoff/asio/tree/master/asio) 库（据说会作为C++20的网络标准库）也是使用的这种模型，可能是为了兼容windows。

这2种模型最本质的区别是：**Reactor使用的是同步非阻塞I/O，而Preactor使用的是异步（非阻塞？）I/O**（需要操作系统支持），当下Preactor用的不多，**Linux/MacOs下主要以Reactor为主**。

##### Reactor(unix)

简单的来说，Reactor模型就是 **non-blocking IO** + **IO Multiplexing**，它的本质是 [事件驱动编程](http://www.blogjava.net/xyz98/archive/2008/11/24/239393.html) 的一种实现，事件驱动在各种界面开发（Web/Windows/iOS/Android）下会经常用到，比如html中 **input（按钮）** 提供了 **[onclick()](https://www.w3school.com.cn/tags/html_ref_eventattributes.asp)** 事件，当鼠标点击时，则由浏览器回调指定的函数进行相关的操作。  

**java.util.concurrent** 包的作者Doug Lea，在 [《Scalable IO in Java》](http://gee.cs.oswego.edu/dl/cpjslides/nio.pdf)（[翻译](https://www.cnblogs.com/dafanjoy/p/11217708.html)） 一篇分享中对reactor进行了归纳，主要有3种使用方式：
- **单Reactor单线程模型**：基础模型，只有1个线程处理事件循环和分发事件，事件处理函数中不允许出现阻塞。**优点是简单，不涉及并发问题。缺点是无法充分利用CPU，吞吐量相对多线程模型较小**。
- **单Reactor多线程模型**：用的比较多的模型，在事件处理部分通过线程池方式，**可以充分利用CPU资源**。
- **多Reactor多线程模型**：未知，待补充。

根据《Pattern-oriented software architecture. Volume 2》中对Reactor的解释，主要包含以下5个部分：
![event-loop-reactor-participants.png](../images/chapter2/event-loop-reactor-participants.png)
- **Handle**（句柄集事件源）：在Linux中指文件描述符，以socket fd举例，其上的I/O事件由操作系统触发，这样我们可以使用 **accept()** 或者 **recv()/read()** 进行新连接的建立和数据收发处理。
- **Synchronous Event Demultiplexer**（同步事件多路分发器）：通常指select/poll,epoll等I/O多路复用，程序首先将Handle（句柄）以及对应的事件注册到Synchronous Event Demultiplexer上；当有事件到达时，Synchronous Event Demultiplexer就会通知Reactor调用事件处理程序进行处理。
- **Reactor**（反应器）：提供注册和移除事件的功能，以及执行事件循环，不停的接收系统触发的事件，当有事件进入"就绪"状态时，调用注册事件的回调函数处理事件。
- **Event Handler**（事件处理程序）：定义了一个接口（回调函数），主要是给具体事件处理程序传递事件数据。
- **Concrete Event Handler**（具体事件处理程序）：事件的真正处理者，将处理结果写入到句柄上，返回给调用者。

在介绍这种模式之前，我们先来看一下EventLoop。

###### EventLoop（事件循环）

![waht-event-loop.png](../images/chapter2/waht-event-loop.png)

EventLoop，也就是事件循环，主要的功能是：
1. **封装epoll**，对外提供类似 **run()** 函数（libevent是event_base_dispatch），其内是一个无限循环，不停的调用 **epoll_wait()**，处理I/O事件，回调到对应的socket fd对象进行处理。示例代码如下（来自于muduo/EventLoop.cc）：

2. 处理Socket I/O事件的同时，**提供定时器任务的支持**。[高性能定时器的实现可以参考这里](https://www.zhihu.com/zvideo/1339940511960944641)。

下面，我们通过一段具体的代码来看一下什么叫事件循环（来自于TeamTalk，server/src/base/EventDispatch.cpp）：
```c++
void CEventDispatch::StartDispatch(uint32_t wait_timeout)
{
    struct epoll_event events[1024];
    int nfds = 0;

    if(running)
        return;
    running = true;
    
    while (running)
    {
        // 封装epoll_wait()，等待Socket I/O事件
        nfds = epoll_wait(m_epfd, events, 1024, wait_timeout);
        for (int i = 0; i < nfds; i++)
        {
            int ev_fd = events[i].data.fd;
            CBaseSocket* pSocket = FindBaseSocket(ev_fd);
            if (!pSocket)
                continue;
            
            #ifdef EPOLLRDHUP
            if (events[i].events & EPOLLRDHUP)
            {
                //log("On Peer Close, socket=%d, ev_fd);
                pSocket->OnClose();
            }
            #endif
            // Commit End

            if (events[i].events & EPOLLIN)
            {
                //log("OnRead, socket=%d\n", ev_fd);
                pSocket->OnRead();
            }

            if (events[i].events & EPOLLOUT)
            {
			    //log("OnWrite, socket=%d\n", ev_fd);
                pSocket->OnWrite();
            }

            if (events[i].events & (EPOLLPRI | EPOLLERR | EPOLLHUP))
            {
                //log("OnClose, socket=%d\n", ev_fd);
                pSocket->OnClose();
            }

            pSocket->ReleaseRef();
        }

        // 处理定时器任务
        _CheckTimer();
        _CheckLoop();
    }
}
```

看完代码，你可能会有疑惑，为什么要设计这么模块？这就涉及到接下来说的Reactor模型了，**主要的目的是：为了支持多Reactor**。

###### 单Reactor单线程模型

###### 单Reactor多线程模型

###### 多Reactor多线程模型

参考：
- [《Scalable IO in Java》译文](https://www.cnblogs.com/dafanjoy/p/11217708.html)
- [Java-彻底弄懂netty-程序员必须了解的Reactor模式-知识铺](https://baijiahao.baidu.com/s?id=1642204240129683339&wfr=spider&for=pc)
- [深入理解Reactor 网络编程模型](https://zhuanlan.zhihu.com/p/93612337)
- [【NIO系列】——之Reactor模型](https://my.oschina.net/u/1859679/blog/1844109)
- [Proactor 与 Reactor](https://segmentfault.com/a/1190000018331509)

##### Preactor(windows)

### 延伸阅读
#### 关于共享内存
#### 关于用户态TCP
#### windows下的I/O复用：IOCP

## 实现

### 数据模型
#### connect, user模型

### C++实现
### Go实现
### Java实现

## 改进

### 万级别的架构：单机

### 十万级别的架构：多机

### 百万级别的架构：集群

### 万到百万的挑战

#### 三高：高性能、高可用、高并发

#### 高可用技术

#### 高并发技术

- 并发编程
- 协程
- 无锁编程
- 缓存技术
- ……

#### 连接"桶"

#### 分布式集群技术