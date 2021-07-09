/** @file protocol.h
  * @brief 
  * @author teng.qing
  * @date 2021/7/7
  */

#ifndef UDP_CHAT_V2_PROTOCOL_H
#define UDP_CHAT_V2_PROTOCOL_H

#include <string>

// 消息类型，c++11，限定作用域，参考：https://www.cnblogs.com/moodlxs/p/10174533.html
// 注意：enum class MsgType => CmdID，更符和其用途
enum class CmdID {
    kMsgData = 0x100, // 代表这是消息内容
    kMsgAck = 0x101,  // 代表这是确认

    kLoginReq = 0x200,   // 登录请求
    kLoginResp = 0x201,  // 登录响应

    kLoginOut = 0x202,   // 注销通知，无需回复

    kQueryUserListReq = 0x300,  // 查询用户列表请求
    kQueryUserListResp = 0x301, // 查询用户列表响应
};

struct Header {
    int32_t len;  // 数据包长度，不包括头部
    int32_t cmd;  // 命令ID，对应枚举CmdID
};

// 代表一个完整的包
//struct Packet {
//    Header header;
//    char body[];    // c99，柔性数组，必须是最后一个字段，不占内存空间，大小取决于Header中的len
//};

// 登录请求
struct LoginReq {
  // cmd=0x200
  char user_name[32];
  char user_pwd[32];
};

// 登录响应
struct LoginRes {
  // cmd=0x201
  int32_t user_id;
};

// 注销通知
struct LoginOut {
  // cmd=0x202
  int32_t user_id;
};

struct UserInfo {
  int32_t user_id;
};

// 用户列表请求
struct UserListReq {
  // cmd=0x300
  int32_t user_id;
};

// 用户列表响应
struct UserListRes {
  // cmd=0x300
  int32_t list_count; // 列表个数
  char user_id[];     // 字节长度：sizeof(UserListReq)*list_count，需要自己手动解析
};

// 聊天消息
struct Message {
   int32_t from_user_id;      // 来自那个用户ID
   int32_t to_user_id;        // 发给那个用户ID
   std::string text;          // 文本内容
};

// 聊天消息ACK
struct MessageAck {
  int32_t from_user_id; // 来自那个用户ID
  int32_t to_user_id;   // 发给那个用户ID
};

#endif //UDP_CHAT_V2_PROTOCOL_H
