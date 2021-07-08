#include <iostream>

#include "pb/chat.base.pb.h"
#include "pb/chat.msg.pb.h"

/** @fn test_serialize
  * @brief 测试基本用法，反序列化（转化成二进制）和序列化（二进制转换成对象）
  * @return
  */
void test_serialize() {
    std::cout << "test_serialize" << std::endl;

    chat::base::MsgType type = chat::base::MsgType::kMsgTypeUnknown;

    // 序列化
    chat::msg::MessageData data = {};
    // 赋值，通过set_xxx
    data.set_msg("hello protobuf3");
    data.set_to(8007);
    data.set_msg_type(chat::base::MsgType::kMsgTypeText);

    char attach[] = {'a', 'b', 'c', 'd'};
    data.set_attach(attach);

    // data.ByteSizeLong() 返回结构体内存大小
    int struct_len = data.ByteSizeLong();
    char *buffer = new char[struct_len];
    data.SerializeToArray(buffer, struct_len);

    // 此时，就可以通过socket发送出去
    // 假设，现在通过socket接收的数据存放到了buffer中

    // 反序列化
    chat::msg::MessageData newData = {};
    if (!newData.ParseFromArray(buffer, struct_len)) {
        std::cout << "ParseFromArray error:" << std::endl;
    }

    // 取值，通过xxx()
    std::cout << "data.to=" << data.to() << " ,new.to=" << newData.to() << std::endl;
    std::cout << "data.msg=" << data.msg() << " ,new.msg=" << newData.msg() << std::endl;
    std::cout << "data.msg_type=" << data.msg_type() << " ,new.msgType=" << newData.msg_type() << std::endl;
    std::cout << "data.attach=" << data.attach() << " ,new.attach=" << newData.attach() << std::endl;

    delete[] buffer;
};

/** @fn test_list
  * @brief 高级功能：可以定义列表，然后序列化和反序列化
  * @return
  */
void test_list_and_optional() {
    std::cout << "test_list_and_optional" << std::endl;

    chat::msg::MessageListRsp msg_list;
    for (int i = 0; i < 5; i++) {
        auto it = msg_list.add_msg_list(); // 添加一个元素，返回指针，然后赋值
        it->set_to(i);
        it->set_msg("hello");
        it->set_msg_type(chat::base::MsgType::kMsgTypeText);

        // 测试可选字段
        if (i % 2 == 0) {
            it->set_attach("this is attach" + std::to_string(i));
        }
    }

    char *buffer = new char[msg_list.ByteSizeLong()];
    msg_list.SerializeToArray(buffer, msg_list.ByteSizeLong());

    chat::msg::MessageListRsp new_msg_list;
    assert(new_msg_list.ParseFromArray(buffer, msg_list.ByteSizeLong()));

    for (int i = 0; i < new_msg_list.msg_list_size(); ++i) {
        auto &item = new_msg_list.msg_list(i);

        if (!item.has_attach()) {
            std::cout << "to=" << item.to() << " ,msg=" << item.msg() << std::endl;
        } else {
            std::cout << "to=" << item.to() << " ,msg=" << item.msg() << ", attach=" << item.attach() << std::endl;
        }
    }

    delete[] buffer;
}


int main() {
    test_serialize();

    std::cout << std::endl;
    test_list_and_optional();

    return 0;
}