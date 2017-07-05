//
// Created by zhou rui on 17/6/29.
//

#ifndef DREAM_P2PCLIENTSESSION_H
#define DREAM_P2PCLIENTSESSION_H


#include <ctime>
#include <string>
#include "../util/public/platform.h"

namespace Dream {

    class P2PClientSession {

    public:
        P2PClientSession(const std::string& client_id, const std::string& client_pwd = "")
        : _client_id(client_id), _client_pwd(client_pwd) {
            update_last_active_time();
        }

        ~P2PClientSession() = default;

        void update_last_active_time() {
            _lastActiveTime = getCurrentTimeStamp();
        }

        time_t last_active_time() const {
            return _lastActiveTime;
        }

        void update_nat_addr(const std::string& addr) {
            _nat_addr = addr;
        }

        const std::string nat_addr() const {
            return _nat_addr;
        }

        void update_nat_port(unsigned short port) {
            _nat_port = port;
        }

        unsigned short nat_port() const {
            return _nat_port;
        }

        void update_internal_addr(const std::string& addr) {
            _internal_addr = addr;
        }

        const std::string internal_addr() const {
            return _internal_addr;
        }

        void update_internal_port(unsigned short port) {
            _internal_port = port;
        }

        unsigned short internal_port() const {
            return _internal_port;
        }

    private:
        std::string _client_id;  // 客户端唯一编号
        std::string _client_pwd;  // 客户端密码
        time_t _lastActiveTime;  // 最后活动时间

        std::string _nat_addr;  // 客户端外部地址
        unsigned short _nat_port;  // 客户端外部端口

        std::string _internal_addr;  // 客户端内部地址
        unsigned short _internal_port;  // 客户端外部地址
    };

}


#endif //VMSDKDEMO_ANDROID_P2PCLIENTSESSION_H
