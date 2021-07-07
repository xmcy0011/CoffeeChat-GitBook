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

/** @fn
  * @brief
  * @param [in]aa:
  * @return
  */
struct Message {
    int32_t type;       // see MsgType
    char data[200]; // 对于不定长的字符串，我们只能规定一个长度
};

struct Header {
    int len;
    int cmd;
};

struct Message {
    int32_t type;       // see MsgType
    char data[32];      // 对于不定长的字符串，我们只能规定一个长度
    int to_user;        // 增加1个字段。
};

struct Packet {
    Header header;
    char body[];    // c99，柔性数组，必须是最后一个字段，不占内存空间
};

#endif //UDP_CHAT_V2_PROTOCOL_H
