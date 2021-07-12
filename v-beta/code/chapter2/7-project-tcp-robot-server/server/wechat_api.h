/** @file wechat_api.h
  * @brief 
  * @author teng.qing
  * @date 2021/7/12
  */

#ifndef TCP_ROBOT_SERVER_WECHAT_API_H_
#define TCP_ROBOT_SERVER_WECHAT_API_H_

#include <string>
#include <atomic>

/** @class wechat_api
  * @brief
  */
class WeChatApi {
public:
    /** @fn getAnswer
      * @brief 获取问答，自动获取签名和处理签名超时的情况
      * @param [in]aa:
      * @return
      */
    static bool getAnswer(const std::string &question, std::string &answer);

private:
    /** @fn getSignature
      * @brief 获取签名，根据用户名、ID
      * @param [in]token: 机器人的唯一标志
      * @return
      */
    static bool getSignature(const std::string &token, std::string &signature, int &expires_in);

    /** @fn
      * @brief
      * @param [in]aa:
      * @return
      */
    static bool query(const std::string &signature, const std::string &question, std::string &answer);

private:
    static std::string signature_;
    static std::atomic_int64_t signature_time_; // 上一次获取签名时间
    static int expires_in_; // 签名过期时间
};


#endif //TCP_ROBOT_SERVER_WECHAT_API_H_
