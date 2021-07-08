/** @file protocol.h
  * @brief 
  * @author teng.qing
  * @date 2021/7/7
  */

#ifndef UDP_CHAT_V2_PROTOCOL_H
#define UDP_CHAT_V2_PROTOCOL_H

// 消息类型，c++11，限定作用域，参考：https://www.cnblogs.com/moodlxs/p/10174533.html
enum class MsgType {
    kMsgData = 1, // 代表这是消息内容
    kMsgAck,  // 代表这是确认
};

struct Header {
    int32_t len;
    int32_t cmd;
};

// 代表一个完整的包
struct Packet {
    Header header;
    char body[];    // c99，柔性数组，必须是最后一个字段，不占内存空间，大小取决于Header中的len
};

#endif //UDP_CHAT_V2_PROTOCOL_H
