原文：https://mp.weixin.qq.com/s?__biz=MzI1ODY0NjAwMA==&mid=2247483756&idx=1&sn=a8e3303bc573b1acaf9ef3862ef89bdd&chksm=ea044bf3dd73c2e5dcf2c10202c66d6143ec866205e9230f974fbc0b0be587926699230b6b18&scene=21#wechat_redirect

# 1 服务器端设计

## 1.1 总体架构
总体架构包括5个层级，具体内容如下图。
![images/im-design-1](images/im-design-1.webp)

### 1.1.1 用户端
移动端重点是移动端，支持IOS/Android系统，包括IM App，嵌入消息功能的瓜子App，未来还可能接入客服系统。

### 1.1.2 用户端API
针对TCP协议，提供IOS/Android开发SDK。对于H5页面，提供WebSocket接口

### 1.1.3 接入层
接入层主要任务是保持海量用户连接（接入）、攻击防护、将海量连接整流成少量TCP连接与逻辑层通讯。

### 1.1.4 逻辑层
逻辑层负责IM系统各项功能的核心逻辑实现。包括单聊（c2c）、上报(c2s)、推送(s2c)、群聊(c2g)、离线消息、登录授权、组织机构树等等内容。

### 1.1.5 存储层
存储层负责缓存或存储IM系统相关数据，主要包括用户状态及路由（缓存），消息数据（MySQL也可采用NoSql，如MangoDB），文件数据（文件服务器）。

## 1.2 逻辑结构

### 1.2.1 核心结构
核心结构部分描述IM系统核心组件及其关系。结构图如下。
![im-design-1](images/im-design-2.jpeg)

客户端从Iplist服务获取接入层IP地址（也可采用域名的方式解析得到接入层IP地址），建立与接入层的连接（可能为短连接），从而实现客户端与IM服务器的数据交互；业务线服务器可以通过服务器端API建立与IM服务器的联系，向客户端推送消息；客户端上报到业务服务器的消息，IM服务器会通过mq投递给业务服务器。

### 1.2.2 tcp接入核心流程

#### 1.2.2.1 登录授权(auth)
![im-design-login.webp](images/im-design-login.webp)
1、客户端通过统一登录系统实现登录，得到token。
2、客户端用uid和token向msg-gate发起授权验证请求。
3、msg-gate同步调用msg-logic的验证接口
4、msg-logic请求sso系统验证token合法性
5、msg-gate得到登录结果后，设置session状态，并向客户端返回授权结果。

#### 1.2.2.2 登出(logout)
![im-design-login](images/im-design-logout.png)
1、客户端发起logout请求，msg-gate设置对应Peer为未登录状态。
2、	Msg-gate给客户端一个ack响应。
3、	Msg-gate通知msg-logic用户登出。

#### 1.2.2.3	踢人(kickout)
用户请求授权时，可能在另一个设备（同类型设备）开着软件处于登录状态。这种情况需要系统将那个设备踢下线。
![im-design-login.webp](images/im-design-kick.webp)
1-5步，参看Auth流程。
6、	Logic检索Redis，查看是否该用户在其他地方登录。
7、	如果在其他地方登录，发起kickout命令。（如果没有登录，整个流程结束）
8、	Gate向用户发起kickout请求，并在短时间内（确保客户端收到kickout数据）关闭socket连接。

#### 1.2.2.4 上报(c2s)
 
![im-design-c2s](images/im-design-c2s.png)
1、	客户端向gate发送数据
2、	Gate回一个ack包，向客户端确认已经收到数据
3、	Gate将数据包传递给logic
4、	Logic根据数据投递目的地，选择对应的mq队列进行投递
5、	业务服务器得到数据

#### 1.2.2.5	推送(s2c)
![im-design-s2c](images/im-design-s2c.webp)
1、	业务线调用push数据接口sendMsg
2、	Logic向redis检索目标用户状态。如果目标用户不在线，丢弃数据（未来可根据业务场景定制化逻辑）；如果用户在线，查询到用户连接的接入层gate
3、	Logic向用户所在的gate发送数据
4、	Gate向用户推送数据。（如果用户不在线，通知logic用户不在线）
5、	客户端收到数据后向gate发送ack反馈
6、	Gate将ack信息传递给logic层，用于其他可能的逻辑处理（如日志，确认送达等）

#### 1.2.2.6	单对单聊天(c2c)
![im-design-c2c](images/im-design-c2c.jpeg)
1、	App1向gate1发送信息（信息最终要发给App2）
2、	Gate1将信息投递给logic
3、	Logic收到信息后，将信息进行存储
4、	存储成功后，logic向gate1发送ack
5、	Gate1将ack信息发给App1
6、	Logic检索redis，查找App2状态。如果App2未登录，流程结束
7、	如果App2登录到了gate2，logic将消息发往gate2
8、	Gate2将消息发给App2（如果发现App2不在线，丢弃消息即可，这种概率极低，后续离线消息可保证消息不丢）
9、	App2向gate2发送ack
10、Gate2将ack信息发给logic
11、Logic将消息状态设置为已送达。
注：在第6步和第7步之间，启动计时器（DelayedQueue或哈希环，时间如5秒），计时器时间到后，探测该条消息状态，如果消息未送达，考虑通过APNS、米推、个推进行推送

#### 1.2.2.7 群聊(c2g)
采用`扩散写`（而非扩散读）的方式。
群聊是多人社交的基本诉求，一个群友在群内发了一条消息：
（1）在线的群友能第一时间收到消息
（2）离线的群友能在登陆后收到消息
由于“消息风暴扩散系数”的存在，群消息的复杂度要远高于单对单消息。
群基础表：用来描述一个群的基本信息
im_group_msgs(group_id, group_name,create_user, owner, announcement, create_time)
群成员表：用来描述一个群里有多少成员
im_group_users(group_id, user_id)
用户接收消息表：用来描述一个用户的所有收到群消息（与单对单消息表是同一个表）
im_message_recieve（msg_id,msg_from,msg_to, group_id，msg_seq, msg_content, send_time, msg_type, deliverd, cmd_id）
用户发送消息表：用来描述一个用户发送了哪些消息
im_message_send (msg_id,msg_from,msg_to, group_id，msg_seq, msg_content, send_time, msg_type, cmd_id)
业务场景举例：
（1）一个群中有x,A,B,C,D共5个成员，成员x发了一个消息
（2）成员A与B在线，期望实时收到消息
（3）成员C与D离线，期望未来拉取到离线消息
群聊流程如下图所示
![im-design-c2g](images/im-design-c2g.webp)
1、X向gate发送信息（信息最终要发给这个群，A、B在线）
2、Gate将消息发给logic
3、存储消息到im_message_send表，按照msg_from水平分库
4、回ack
5、回ack
6、Logic检索数据库（需要使用缓存），获得群成员列表
7、存储每个用户的消息数据（用户视图），按照msg_to水平分库(`并发、批量写入`)。
8、查询用户在线状态及位置
9、Logic向gate投递消息
10、Gate向用户投递消息
11、App返回收到消息的ack信息
12、Gate向logic传递ack信息
13、向缓存（Hash）中更新收到ack的时间。然后在通过一个定时任务，每隔一定时间，将数据更新到数据库（注意只需要写入时间段内有变化的数据）。

#### 1.2.2.8	拉取离线消息
下图中，将gate和logic合并为im-server。拉取离线消息流程如下。 
![im-design-pull-offline-msg](images/im-design-pull-offline-msg.jpeg)
1、	App端登录成功后（或业务触发拉取离线消息），向IM系统发起拉离线消息请求。传递3个主要参数，uid表明用户；msgid表明当前收到的最大消息id（如果没收到过消息，或拿不到最大消息id则msgid=0）即可；size表示每次拉取条数（这个值也可以由服务器端控制）。
2、	假设msgid==0，什么都不做。（参看第6步骤）
3、	Im-server查询用户前10条离线消息
4、	将离线消息推给用户。假设这10条离线消息最大msgid=110。
5、	App得到数据，判断得到的数据不为空（表明可能没有拉完离线数据，不用<10条做判断拉完条件,因为服务端需要下下次拉离线的请求来确定这次数据已送达），继续发起拉取操作。Msgid=110(取得到的离线消息中最大的msgid)。
6、	Im-server删除该用户msgid<110的离线消息（或者标记为已送达）。
7、	查询msgid>110的钱10条离线数据。
8、	返回给App
……
N-1、查询msgid>140的离线数据，0条（没有离线数据了）。
N  、将数据返回App，App判断拉取到0条数据，结束离线拉取过程。

### 1.2.3 PUSH
ISO采用APNS；Android真后台保活，同时增加米推、个推。
基本思路：push提示信息，App通过拉离线获得真实消息。
另附文档说明此问题。

# 2 协议设计

## 2.1 TCP数据协议
TCP的数据协议如下图所示。包括header和body两部分。
![im-design-protocol-header](images/im-design-protocol-header.webp)

消息头总共20个字节，具体信息如下表。
![im-design-protocol-body](images/im-design-protocol-body.webp)

## 2.2	TCP消息体设计
消息体协议采用ProtocolBuffer（谷歌）协议，版本3.0.0，该协议在序列化效率、压缩、可扩展方面都具有优势。协议条目见附录11.1.1TCP协议命令清单。以下为主要流程涉及的协议

### 2.2.1 认证(auth) 
![im-design-protocol-auth.webp](images/im-design-protocol-auth.webp)

### 2.2.2 登出(logout) 
![im-design-protocol-logout.png](images/im-design-protocol-logout.png)

### 2.2.3 踢人(kickout) 
![im-design-protocol-kick](images/im-design-protocol-kick.png)

### 2.2.4 心跳（keepalive,noop）
心跳包消息体为空。

### 2.2.5	单对单聊天（c2c）
![im-design-protocol-c2c-push](images/im-design-protocol-c2c-push.png)
![im-design-protocol-c2c-send](images/im-design-protocol-c2c-send.webp)

### 2.2.6	群聊（c2g）
![im-design-protocol-c2g-push](images/im-design-protocol-c2g-push.webp)
![im-design-protocol-c2g-send](images/im-design-protocol-c2g-send.webp)

### 2.2.7	拉离线（pull）
![im-design-protocol-pull](images/im-design-protocol-pull.webp)

### 2.2.8	控制类（ctrl） 
![im-design-protocol-ctrl1](images/im-design-protocol-ctrl1.webp)
![im-design-protocol-ctrl2](images/im-design-protocol-ctrl2.webp)

# 3 存储设计
## 3.1	MySQL数据库
MySQL数据库采用utf8mb4编码格式（emoji字符问题）
### 3.1.1	主要表结构
#### 3.1.1.1	发送消息表
保存某个用户发送了哪些消息，用于复现用户聊天场景（消息漫游功能需要）。
![im-design-storage-sendmsg](images/im-design-storage-sendmsg.webp)

#### 3.1.1.2	推送消息表
保存某个用户收到了哪些消息
![im-design-storage-pushmsg](images/im-design-storage-pushmsg.jpeg)

#### 3.1.1.3	群相关表
群基本信息表
![im-design-storage-group1](images/im-design-storage-group1.webp)

群用户关系表
![im-design-storage-group2](images/im-design-storage-group2.webp)

### 3.1.2 水平分库
![im-design-storage-send-recv](images/im-design-storage-send-recv.webp)

## 3.2	Redis缓存

### 3.2.1	用户状态及路由信息
Redis缓存以uid为key，检索channel(socketid)，last_packet_time等。
Gate层，session以channel(socketed)为key，检索uid，及其他信息。
交互接口：`gate->logic`，通过将channel转换为uid作为key。
`logic->gate`，将uid转换为channel作为key。

### 3.2.2	其他缓存信息
你觉得该怎么存就怎么存。

## 3.3	文件及图片存储
采用商用云存储。

## 3.4	数据归档
可考虑采用HBase,HDFS作为数据归档，或者相关云存储服务。

安全部分略，其他非核心功能略

相关阅读
- [《IM系统的SESSION结构》](https://mp.weixin.qq.com/s?__biz=MzI1ODY0NjAwMA==&mid=2247483714&idx=1&sn=8f12924038a8d7857c3c9a4fbff8bbac&chksm=ea044bdddd73c2cb16c500fc9077074435216330e8b9b41daaf19c18261249fef1280c9f0566&scene=21#wechat_redirect)
- [《IM系统如何调试TCP协议》](https://mp.weixin.qq.com/s?__biz=MzI1ODY0NjAwMA==&mid=2247483692&idx=1&sn=586b4425d402f6d990adc36a88a25f89&chksm=ea044bb3dd73c2a5ca7100ebbd19bffaba7c72d24e2f108c4d8bb06f7b0c49805a24f07332b1&scene=21#wechat_redirect)
- [《NAT是怎么回事》](https://mp.weixin.qq.com/s?__biz=MzI1ODY0NjAwMA==&mid=2247483703&idx=1&sn=6a8f180d7cbdb5cdd3c26c67997d24fd&chksm=ea044ba8dd73c2be31089853f2edd91ecb33d732f2013917d12d8de3b4e9499ebe3a9e9b7b62&scene=21#wechat_redirect)
- [《视频聊天功能如何穿透NAT》](https://mp.weixin.qq.com/s?__biz=MzI1ODY0NjAwMA==&mid=2247483707&idx=1&sn=2b842aac588e29290a99eb1f30555f54&chksm=ea044ba4dd73c2b221ef74c26230929ce6c18611d64ec5b2e1122e51e4f8e1e2ef527f7ebde2&scene=21#wechat_redirect)
- [《IM移动端怎么搜索本地聊天记录》](https://mp.weixin.qq.com/s?__biz=MzI1ODY0NjAwMA==&mid=2247483725&idx=1&sn=3aec201e78b30c6cd7988dfbf4180126&chksm=ea044bd2dd73c2c450d6c4dec29a8e7fbfbc2e55f110955ae151516d845480b0be3146ad1c41&scene=21#wechat_redirect)