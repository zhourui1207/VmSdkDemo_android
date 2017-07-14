/*
 * CrossPlatform.h
 *
 *  Created on: 2016年9月12日
 *      Author: zhourui
 */

#ifndef PLATFORM_H_
#define PLATFORM_H_

#include <chrono>
#include <stdint.h>
#include <string>

#define ENCODE_INT(pBuf, nValue, nPos) do \
    {\
      unsigned uTmp = htonl(nValue);\
      memcpy(pBuf, &uTmp, sizeof(int));\
      nPos += sizeof(int);\
    } while (0);

#define DECODE_INT(pBuf, nValue, nPos) do \
    {\
      memcpy(&nValue, pBuf, sizeof(int));\
      nValue = ntohl(nValue);\
      nPos += sizeof(int);\
    } while (0);

#define ENCODE_SHORT(pBuf, shortValue, nPos) do \
    {\
      unsigned uTmp = htons(shortValue);\
      memcpy(pBuf, &uTmp, sizeof(short));\
      nPos += sizeof(short);\
    } while (0);

#define DECODE_SHORT(pBuf, shortValue, nPos) do \
    {\
      memcpy(&shortValue, pBuf, sizeof(short));\
      shortValue = ntohs(shortValue);\
      nPos += sizeof(short);\
    } while (0);

#define ENCODE_STRING(pBuf, sValue, nPos) do \
    {\
      memcpy(pBuf, sValue.c_str(), std::size_t(sValue.length() + 1)); \
      nPos += (sValue.length() + 1);\
    } while(0);

#define DECODE_STRING(pBuf, sValue, nPos) do \
    {\
      sValue = pBuf; \
      nPos += (sValue.length() + 1);\
    } while(0);


inline time_t getCurrentTimeStamp() {
    std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> tp = std::chrono::time_point_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now());
    auto tmp = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());
    return (time_t) tmp.count();
}

inline bool ipv4_2int(const std::string& ipStr, unsigned& ipInt) {
    unsigned int findBegin = 0;
    char tmpIp[4];
    std::string tmpStr = ipStr;
    for (int i = 0; i < 4; ++i) {
        int pointPos = tmpStr.find(".", findBegin);
        if (pointPos != std::string::npos) {
            std::string number = tmpStr.substr(findBegin, (unsigned int) pointPos);
            findBegin = (unsigned int) (pointPos + 1);
            int n = atoi(number.c_str());
            tmpIp[i] = (char) n;
        } else {
            if (i < 3) {
                return false;
            } else {
                std::string number = tmpStr.substr(findBegin, std::string::npos);
                int n = atoi(number.c_str());
                tmpIp[i] = (char) n;
            }
        }
    }
    int pos = 0;
    ENCODE_INT(tmpIp, ipInt, pos);
    return true;
}

inline unsigned rangeRand(unsigned from, unsigned to) {
    return (rand() % (to - from + 1)) + from;
}

#ifdef _WIN32
#include <windows.h>
#else

#include <arpa/inet.h>
#include <unistd.h>

#endif

/** sleep 跨平台 */
#if _WIN32
inline void safeSleep(uint32_t ms) {
  ::Sleep(ms);
}
#else

inline void safeSleep(uint32_t ms) {
    usleep(ms * 1000);
}

#endif

#ifdef _ANDROID

#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#define LOGD(TAG, ...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__) // 定义LOGD类型
#define LOGI(TAG, ...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__) // 定义LOGI类型
#define LOGW(TAG, ...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__) // 定义LOGW类型
#define LOGE(TAG, ...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__) // 定义LOGE类型
#define LOGF(TAG, ...) __android_log_print(ANDROID_LOG_FATAL, TAG, __VA_ARGS__) // 定义LOGF类型
#else
#define LOGD(TAG, ...) printf(__VA_ARGS__)
#define LOGI(TAG, ...) printf(__VA_ARGS__)
#define LOGW(TAG, ...) printf(__VA_ARGS__)
#define LOGE(TAG, ...) printf(__VA_ARGS__)
#define LOGF(TAG, ...) printf(__VA_ARGS__)
#endif

#endif /* CROSSPLATFORM_H_ */
