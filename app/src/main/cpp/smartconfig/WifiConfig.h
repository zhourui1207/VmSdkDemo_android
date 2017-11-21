//
// Created by zhou rui on 2017/11/9.
//

#ifndef DREAM_WIFICONFIG_H
#define DREAM_WIFICONFIG_H

#include <string>
#include <memory>
#include <thread>
#include <condition_variable>
#include "../util/AsioUdpServer.h"
#include "smart_config.h"

namespace Dream {

    class WifiConfig {
    private:
        const static size_t BUFLEN = 64;
        const static size_t SSIDLEN = 1;
        const static size_t PWDLEN = 1;
        const static size_t ENCYLEN = 1;
        const static size_t MASKLEN = 5;
        const static size_t DATALEN = BUFLEN - MASKLEN - PWDLEN - SSIDLEN - ENCYLEN;

        const static uint8_t MASKMIN = 0x01;
        const static uint8_t MASKMAX = 0xff;
        const static uint8_t MASKASC = 0x20;

        const static size_t BUF_SIZE = 4096;

        const char *GROUP_ADDR = "224.119.17.0"; //组播地址
        const static uint16_t GROUP_PORT = 3333;  // 组播端口
        const static uint16_t LOCAL_PORT = 3333;  // 本地端口

        const char *TAG = "WifiConfig";

    public:

        WifiConfig();

        virtual ~WifiConfig();

        bool config(const std::string &wifiSsid, const std::string &wifiPwd,
                    int64_t packetIntervalMillis = 0, int64_t waitIntervalMillis = 100,
                    int32_t encryType = ENCRY_WPA_TKIP, int32_t keyIndex = WEP_KEY_INDEX_0,
                    int32_t wepPwdLen = WEP_PWD_LEN_64);

        void stop();

    private:
        bool
        generateOriginalPackage(char *buf, const char *wifiSsid, const char *wifiPwd, int encryType,
                                int keyIndx, int pwdLen);

        void stopNoLock();

        void run();

        bool _configing;  // 正在配置
        std::unique_ptr<std::thread> _threadUp;  // 线程
        std::unique_ptr<AsioUdpServer> _udpServerUp;  // udp
        std::mutex _mutex;
        std::condition_variable _condition;

        char _buf[BUFLEN];
        int64_t _packetIntervalMillis;
        int64_t _waitIntervalMillis;
    };

}

#endif //VMSDKDEMO_ANDROID_WIFICONFIG_H
