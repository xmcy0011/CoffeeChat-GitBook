# 引言

本教程根据作者的实践经验以及网络上的一些文章整理而成，主要讲解从0到1打造一款IM系统的方方面面（主要以后端为主，前端和客户端后面有精力了也会补）。

在撰写本教程时，作者负责的某电商IM后台从19年9月份稳定运行至今，没有出现过线上事故（上新需求的时候，某台机器上的服务会crash，因为做了双机部署，所以不影响整体的使用），生产环境大概有30台左右的虚拟（2C4G），平时活跃用户在几千。

这期间遇到的问题，也希望一一归纳总结到本教程内，毕竟记性不好 :)


# 定位

## 目标读者

本书假设读者具有一定的后端开发经验（2-3年以上为好）。

- C++开发者
- Java开发者
- Go开发者

## 本书目的

- 初步掌握IM后端开发相关的技能。
  - 单聊如何实现？
  
  - 群聊如何实现？
  
  - 语音和视频以及文件如何实现？
  
  - 协议如何设计才能省流量，又能兼容老版本？
  
  - 如何存储消息？
  
  - ……等等
  
    
  
- 掌握Linux下的网络编程技术
  
  - select、poll、epoll
  
    
  
- 掌握Protobuf
- 百万级用户量IM的架构
- ……


# 联系我

- 坐标：上海
- 博客：https://www.geek265.com
- 邮箱：xmcy0011@sina.com
- 微信：xuyc1992

# 作品

CoffeeChat：https://github.com/xmcy0011/CoffeeChat ![Github starts](https://img.shields.io/badge/stars-22-yellowgreen)

> opensource im with server(go) and client(flutter+swift)