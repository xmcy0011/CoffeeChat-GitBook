/** @file protocol.h
  * @brief 
  * @author teng.qing
  * @date 2021/7/7
  */

#ifndef UDP_CHAT_V2_PROTOCOL_H
#define UDP_CHAT_V2_PROTOCOL_H

// 消息类型
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

#endif //UDP_CHAT_V2_PROTOCOL_H
