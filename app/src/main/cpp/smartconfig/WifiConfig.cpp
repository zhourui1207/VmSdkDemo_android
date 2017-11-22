//
// Created by zhou rui on 2017/11/9.
//

#include "WifiConfig.h"
#include "../util/public/StringUtil.h"

namespace Dream {

    WifiConfig::WifiConfig()
            : _configing(false), _packetIntervalMillis(0), _waitIntervalMillis(100) {

    }

    WifiConfig::~WifiConfig() {
        stop();
    }

    bool
    WifiConfig::config(const std::string &wifiSsid, const std::string &wifiPwd,
                       int64_t packetIntervalMillis, int64_t waitIntervalMillis,
                       int32_t encryType, int32_t keyIndex, int32_t wepPwdLen) {
        if (wifiSsid.length() == 0) {
            LOGE(TAG, "WifiSSID's length can't be 0!\n");
            return false;
        }

        if (encryType != ENCRY_NONE && encryType != ENCRY_WEP_OPEN &&
            encryType != ENCRY_WEP_SHARED && encryType != ENCRY_WPA_TKIP &&
            encryType != ENCRY_WPA_AES) {
            LOGE(TAG, "EncryType invalid! current is [%d]\n", encryType);
            return false;
        }

        if (keyIndex != WEP_KEY_INDEX_0 && keyIndex != WEP_KEY_INDEX_1 &&
            keyIndex != WEP_KEY_INDEX_2 && keyIndex != WEP_KEY_INDEX_3 &&
            keyIndex != WEP_KEY_INDEX_MAX) {
            LOGE(TAG, "KeyIndex invalid! current is [%d]\n", keyIndex);
            return false;
        }

        if (wepPwdLen != WEP_PWD_LEN_64 && wepPwdLen != WEP_PWD_LEN_128) {
            LOGE(TAG, "WepPwdLen invalid! current is [%d]\n", wepPwdLen);
            return false;
        }

        std::unique_lock<std::mutex> lock(_mutex);
        if (_configing) {  // 如果已经开始了，那么就先停止掉，然后再配置
            _configing = false;
            _condition.notify_all();
            stopNoLock();
        }

        // 正式开始
        if (wifiPwd.length() == 0) {  // 如果没有密码，就强制改变加密类型
            encryType = ENCRY_NONE;
        }

        // utf转gbk
        char wifiSsidGbk[100] = {0};
        char wifiPwdGbk[100] = {0};
        u2g(wifiSsid.c_str(), wifiSsid.length(), wifiSsidGbk, 100);
        u2g(wifiPwd.c_str(), wifiPwd.length(), wifiPwdGbk, 100);

        if (!generateOriginalPackage(_buf, wifiSsidGbk, wifiPwdGbk, encryType, keyIndex,
                                    wepPwdLen)) {
            LOGE(TAG, "Generate original packet failed!\n");
            return false;
        }

        _packetIntervalMillis = packetIntervalMillis > 0 ? packetIntervalMillis : 0;
        _waitIntervalMillis = waitIntervalMillis > 0 ? waitIntervalMillis : 0;

        // 开启udp
        _udpServerUp = std::unique_ptr<AsioUdpServer>(new AsioUdpServer(nullptr, "", LOCAL_PORT));
        _udpServerUp->start_up();

        // 开始发送线程
        _configing = true;
        _threadUp = std::unique_ptr<std::thread>(new std::thread(&WifiConfig::run, this));
        return true;
    }

    void WifiConfig::stop() {
        std::unique_lock<std::mutex> lock(_mutex);
        if (_configing) {
            _configing = false;
            _condition.notify_all();
            stopNoLock();
        }
    }

    void WifiConfig::stopNoLock() {
        // 关闭发送线程
        if (_threadUp.get() && _threadUp->joinable()) {
            _threadUp->detach();
            _threadUp.reset();
        }
        // 关闭udp
        if (_udpServerUp.get()) {
            _udpServerUp->shut_down();
            _udpServerUp.reset();
        }
    }

    bool WifiConfig::generateOriginalPackage(char *buf, const char *wifiSsid, const char *wifiPwd,
                                             int encryType, int keyIndx, int pwdLen) {
        if (strlen(wifiPwd) + strlen(wifiSsid) > DATALEN) {
            LOGE(TAG, "DATALEN error!\n");
            return false;
        }

        //由于保留字节是0，这里需要置位成全0
        memset(buf, 0, BUFLEN);
        //填入对应密码长度位置
        buf[BUFLEN - PWDLEN] = (char) strlen(wifiPwd);
        //填入对应wifi的ssid的长度
        buf[BUFLEN - SSIDLEN - PWDLEN] = (char) strlen(wifiSsid);

        //在WEP的加密模式下，由其它的含义
        //0~3bit  加密类型
        //4~5bit  wep key index: 0~3索引
        //6bit  wep模式，0为open，1为shared
        //7bit wep密码长度，0为64bit，1为128bit
        if (WEP_ENC_OPEN == encryType) {
            buf[BUFLEN - SSIDLEN - PWDLEN - ENCYLEN] = (char) ((pwdLen << 7) | (WEP_ENC_OPEN << 6) |
                                                               (keyIndx << 4) | encryType);
        }
        if (ENCRY_WEP_SHARED == encryType) {
            buf[BUFLEN - SSIDLEN - PWDLEN - ENCYLEN] = (char) ((pwdLen << 7) |
                                                               (ENCRY_WEP_SHARED << 6) |
                                                               (keyIndx << 4) | encryType);
        } else {
            buf[BUFLEN - SSIDLEN - PWDLEN - ENCYLEN] = (char) encryType;
        }

        memset((void *) (&(buf[BUFLEN - SSIDLEN - PWDLEN - MASKLEN - ENCYLEN])), 0x00, MASKLEN);
        memcpy((void *) (buf), (void *) wifiSsid, strlen(wifiSsid));
        memcpy((void *) (&(buf[strlen(wifiSsid)])), (void *) wifiPwd, strlen(wifiPwd));
        return true;
    }

    void WifiConfig::run() {
        LOGW(TAG, "Send smart JWSmart_config packet begin!\n");

        char sendBuf[BUF_SIZE];
        memset(sendBuf, 0x51, BUF_SIZE);
        while (_configing) {
//            int packetCount = 0;
            for (int i = 0; i < BUFLEN * 4; ++i) {
                //这里得到要填充的数字长度，算法就是
                //将64字节原始数据逐一按高低顺序拆分成4组2bit的int型（4字节）数据，总共256个。
                //（也就是说一个char会变成4个int，也就是只用了int的低16位）
                //再将每个数据索引左移2位加上其本身的值，再加上固定的掩码，得到最终要发送的数据序列。
                //掩码的大小为0x1～0xFF中间的任意值。
                //根据长度填充重0x51
                int packLen = (i << 2) + ((_buf[i / 4] >> (2 * (3 - (i) % 4))) & 0x03) + MASKASC;

                if (_udpServerUp.get()) {
                    _udpServerUp->send(GROUP_ADDR, GROUP_PORT, sendBuf, (size_t) packLen);
                }
//                ++packetCount;

                if (_packetIntervalMillis > 0) {
                    std::unique_lock<std::mutex> lock(_mutex);
                    if (_configing) {
                        _condition.wait_for(lock, std::chrono::milliseconds(_packetIntervalMillis));
                    }
                    if (!_configing) {
                        break;
                    }
                }
            }

            if (_waitIntervalMillis > 0) {
                std::unique_lock<std::mutex> lock(_mutex);
                if (_configing) {
                    _condition.wait_for(lock, std::chrono::milliseconds(_waitIntervalMillis));
                }
            }
        }

        LOGW(TAG, "Send smart JWSmart_config packet end!\n");
    }
}