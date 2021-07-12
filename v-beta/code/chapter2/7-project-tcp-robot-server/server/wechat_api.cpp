/** @file wechat_api.h
  * @brief 
  * @author teng.qing
  * @date 2021/7/12
  */

#include "wechat_api.h"

#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "httplib.h"   // http库，header only
#include "json.hpp"   // json库，header only

const std::string kRobotToken = "DDDyMjCXqZbjDLKFPSQnOon8WmVkWI"; // 机器人Token，使用那个机器人，替换为那个机器人的Token
const std::string kDefaultUserId = "20210712";

std::string WeChatApi::signature_;
std::atomic_int64_t WeChatApi::signature_time_(0);  // 上一次获取签名时间
int WeChatApi::expires_in_(0);                      // 签名过期时间

using json = nlohmann::json;

bool WeChatApi::getSignature(const std::string &token, std::string &signature, int &expires_in) {
    std::string host = "openai.weixin.qq.com";
    std::string url = "https://openai.weixin.qq.com/openapi/sign/" + kRobotToken;
    httplib::SSLClient client(host);

    json j;
    j["username"] = kDefaultUserId;
    j["avatar"] = "";
    j["userid"] = kDefaultUserId;
    std::string body = j.dump();

    auto res = client.Post(url.c_str(), body, "application/json");
    if (res != nullptr && res->status == 200) {
        std::cout << res->body << std::endl;

        json root = json::parse(res->body);
        if (!root.is_null()) {
            signature = root["signature"].get<std::string>();
            expires_in = root["expiresIn"].get<int>();
            std::cout << "signature=" << signature << ",expiresIn=" << expires_in << std::endl;
            return true;
        }
    }

    return false;
}

bool WeChatApi::query(const std::string &signature, const std::string &question, std::string &answer) {
    std::string host = "openai.weixin.qq.com";
    std::string url = "https://openai.weixin.qq.com/openapi/aibot/" + kRobotToken;
    httplib::SSLClient client(host);

    json j;
    j["signature"] = signature;
    j["query"] = question;
    j["env"] = "debug";
    std::string body = j.dump();

    auto res = client.Post(url.c_str(), body, "application/json");
    if (res != nullptr && res->status == 200) {
        std::cout << res->body << std::endl;

        json root = json::parse(res->body);
        if (!root.is_null()) {
            answer = root["answer"].get<std::string>();
            std::cout << "answer = " << answer << std::endl;
            return true;
        }
    }

    return false;
}

bool WeChatApi::getAnswer(const std::string &question, std::string &answer) {
    auto cur_time = time(nullptr);
    if (signature_time_ == 0 || (cur_time - signature_time_) > expires_in_) {
        getSignature(kRobotToken, signature_, expires_in_
        );
        signature_time_ = cur_time;
    }

    return query(signature_, question, answer);
}
