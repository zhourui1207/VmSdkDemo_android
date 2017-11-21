/*
 * CrossPlatform.h
 *
 *  Created on: 2016年9月12日
 *      Author: zhourui
 */

#ifndef PLATFORM_H_
#define PLATFORM_H_

#include <time.h>
#include <arpa/inet.h>
#include <chrono>
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <math.h>

#define ENCODE_INT32(pBuf, nValue, nPos) do \
    {\
      int32_t uTmp = htonl(nValue);\
      memcpy(pBuf, &uTmp, sizeof(int32_t));\
      nPos += sizeof(int32_t);\
    } while (0);

#define DECODE_INT32(pBuf, nValue, nPos) do \
    {\
      memcpy(&nValue, pBuf, sizeof(int32_t));\
      nValue = ntohl(nValue);\
      nPos += sizeof(int32_t);\
    } while (0);

#define ENCODE_INT16(pBuf, shortValue, nPos) do \
    {\
      unsigned uTmp = htons(shortValue);\
      memcpy(pBuf, &uTmp, sizeof(int16_t));\
      nPos += sizeof(int16_t);\
    } while (0);

#define DECODE_INT16(pBuf, shortValue, nPos) do \
    {\
      memcpy(&shortValue, pBuf, sizeof(int16_t));\
      shortValue = ntohs(shortValue);\
      nPos += sizeof(int16_t);\
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


inline uint64_t getCurrentTimeStamp() {
    auto tp = std::chrono::time_point_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now());
    auto tmp = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());
    return (uint64_t) tmp.count();
}

inline uint64_t getCurrentTimeStampMicro() {
    auto tp = std::chrono::time_point_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now());
    auto tmp = std::chrono::duration_cast<std::chrono::microseconds>(tp.time_since_epoch());
    return (uint64_t) tmp.count();
}

inline std::size_t numberOf(const std::string& src, char find) {
    const char* ch = src.c_str();
    std::size_t length = src.length();
    std::size_t number = 0;
    for (std::size_t i = 0; i < length; ++i) {
        if (ch[i] == find) {
            ++number;
        }
    }
    return number;
}

inline bool ipv4_2int(const std::string &ipStr, uint32_t &ipInt) {
    ipInt = 0;

    if (numberOf(ipStr, '.') != 3) {
        return false;
    }

    unsigned int findBegin = 0;
    unsigned char tmpIp[4];
    for (int i = 0; i < 4; ++i) {
        int pointPos = ipStr.find(".", findBegin);
        if (pointPos != std::string::npos) {
            std::string number = ipStr.substr(findBegin, (unsigned int) pointPos);
            findBegin = (unsigned int) (pointPos + 1);
            int n = atoi(number.c_str());
            tmpIp[i] = (unsigned char) n;
        } else {
            if (i < 3) {
                return false;
            } else {
                std::string number = ipStr.substr(findBegin, std::string::npos);
                int n = atoi(number.c_str());
                tmpIp[i] = (unsigned char) n;
            }
        }
    }
    int pos = 0;
//    printf("0x%02x 0x%02x 0x%02x 0x%02x\n", tmpIp[0], tmpIp[1], tmpIp[2], tmpIp[3]);
    DECODE_INT32(tmpIp, ipInt, pos);
    return true;
}

inline bool ipv6_2int128(const std::string &ipStr, uint32_t *ipInt) {
    // 先把数据初始化
    memset(ipInt, 0, 16);

    if (ipStr.empty()) {  // 如果ipStr为空字符串
        return false;
    }

    std::string tmpStr = ipStr;

    // 先判断":"个数
    std::size_t pointNumber = numberOf(tmpStr, ':');
    if (pointNumber > 7) {  // 超过7个，那么就一定错误
        return false;
    }

    if (pointNumber < 7) {  // 小于7个的情况，就要判断有没有缩写
        // 有无缩写
        int doublePointPos = tmpStr.find("::");
        if (doublePointPos == std::string::npos) {  // 没有缩写，就直接返回错误
            return false;
        }
        int nextDoublePointPos = tmpStr.find("::", doublePointPos + 1);
        printf("doublePointPos[%d] unsigned char*[%d]\n", doublePointPos, nextDoublePointPos);
        if (nextDoublePointPos != std::string::npos) {  // 如果缩写不只一个，那么也是不对的
            return false;
        }

        // 缩写只有一个，那么满足要求，需要将缩写部分补齐处理
        int replacePointNumber = 7 - pointNumber;
        tmpStr = tmpStr.insert((unsigned int) doublePointPos, (unsigned int) replacePointNumber, ':');
        printf("tmpStr[%s]\n", tmpStr.c_str());
    }

    unsigned char tmpIp[16] = {0};
    unsigned int findBegin = 0;
    for (int i = 0; i < 8; ++i) {
        int pointPos = tmpStr.find(":", findBegin);
        if (pointPos != std::string::npos) {
            // FF01
            std::string number = tmpStr.substr(findBegin, (unsigned int) pointPos - findBegin);
//            printf("pointPos[%d], number[%s]\n", pointPos, number.c_str());
            findBegin = (unsigned int) (pointPos + 1);
            // 如果字符串长度大于4，那么就不是ipv6格式
            std::size_t len = number.length();
//            printf("len[%d]\n", len);
            if (len > 4) {
                return false;
            }

            // 从最低位开始填充
            const char *chNumber = number.c_str();
            int k = 4;
            for (int j = len - 1; j >= 0; --j) {
                uint8_t tmp = toupper(chNumber[j]);  // 转换成大写字母比较
                //                    if (tmp < 0x30 || tmp > 0x46) {  // 不在0-F范围内
                if (tmp >= 48 && tmp <= 57) {  // 在1-9范围内
                    tmp = tmp - 48;
                    if ((k--) % 2 != 0) {
                        tmp <<= 4;
                    }
//					printf("tmp[%02x]\n", tmp);
                    *(tmpIp + i * 2 + k / 2) |= (unsigned char) tmp;
//					printf("now[%02x]\n", *(tmpIp + i * 2 + k / 2));
                } else if (tmp >= 65 && tmp <= 70) {  // 在A-F范围内
                    tmp = tmp - 55;
                    if ((k--) % 2 != 0) {
                        tmp <<= 4;
                    }
//					printf("tmp[%02x]\n", tmp);
                    *(tmpIp + i * 2 + k / 2) |= (unsigned char) tmp;
//					printf("now[%02x]\n", *(tmpIp + i * 2 + k / 2));
                } else {
                    return false;
                }
            }
        } else {
            if (i < 7) {
                return false;
            } else {
                std::string number = tmpStr.substr(findBegin, std::string::npos);
//                printf("pointPos[%d], number[%s]\n", pointPos, number.c_str());
                // 如果字符串长度大于4，那么就不是ipv6格式
                std::size_t len = number.length();
//                printf("len[%d]\n", len);
                if (len > 4) {
                    return false;
                }
                // 从最低位开始填充
                const char *chNumber = number.c_str();
                int k = 4;
                for (int j = len - 1; j >= 0; --j) {
                    uint8_t tmp = toupper(chNumber[j]);  // 转换成大写字母比较
//                    if (tmp < 0x30 || tmp > 0x46) {  // 不在0-F范围内
                    if (tmp >= 48 && tmp <= 57) {  // 在1-9范围内
                        tmp = tmp - 48;
                        if ((k--) % 2 != 0) {
//                    		printf("<<\n", tmp);
                            tmp <<= 4;
                        }
//                    	printf("tmp[%02x]\n", tmp);
                        *(tmpIp + i * 2 + k / 2) |= tmp;
//                        printf("now[%02x]\n", *(tmpIp + i * 2 + k / 2));
                    } else if (tmp >= 65 && tmp <= 70) {  // 在A-F范围内
                        tmp = tmp - 55;
                        if ((k--) % 2 != 0) {
//                    		printf("<<\n", tmp);
                            tmp <<= 4;
                        }
//                    	printf("tmp[%02x]\n", tmp);
                        *(tmpIp + i * 2 + k / 2) |= tmp;
//						printf("now[%02x]\n", *(tmpIp + i * 2 + k / 2));
                    } else {
                        return false;
                    }
                }
            }
        }
    }


//    DECODE_INT32(tmpIp, ipInt, pos);
//    printf("end\n");
//    memcpy((unsigned char*)ipInt, tmpIp, 16);
    for (int i = 0; i < 4; ++i) {
        int pos = 0;
        DECODE_INT32(tmpIp + i * 4, *(ipInt + i), pos);
    }

//    printf("end1\n");
    return true;
}

//static unsigned zr_pre_rand = 0;

inline uint32_t rangeRand(uint32_t from, uint32_t to, uint32_t seed = 0) {
    srand(seed == 0 ? (unsigned int) time(NULL) : seed);
    return (rand() % (to - from + 1)) + from;
}

inline int32_t max(int32_t a, int32_t b) {
    return a > b ? a : b;
}

inline int32_t min(int32_t a, int32_t b) {
    return a < b ? a : b;
}

inline int seqcmp(int32_t seq1, int32_t seq2) {
    return (abs(seq1 - seq2) < 0x3fffffff) ? (seq1 - seq2) : (seq2 - seq1);
}

inline int seqlen(int32_t seq1, int32_t seq2) {
    return (seq1 <= seq2) ? (seq2 - seq1 + 1) : (seq2 - seq1 + INT32_MAX + 2);
}

template<typename T>
inline void toArray(const std::vector<T>& list, T* array, size_t array_size) {
    int size = min(list.size(), array_size);
    memset((void*)array, 0, array_size * sizeof(T));
    for (int i = 0; i < size; ++i) {
        array[i] = list[i];
    }
}

inline std::string getHexString(const char* buf, int begin, size_t len) {
    std::string retStr;
    for (size_t i = 0; i < len; ++i) {
        char tmpCh = *(buf + begin + i);
        char tmpStr[10];
        sprintf(tmpStr, "0x%02x ", tmpCh);
        retStr += tmpStr;
    }
    return retStr;
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

#if(DEBUG)
#define LOGD(TAG, ...) printf(__VA_ARGS__)
#else
#define LOGD(TAG, ...)
#endif

#define LOGI(TAG, ...) printf(__VA_ARGS__)
#define LOGW(TAG, ...) printf(__VA_ARGS__)
#define LOGE(TAG, ...) printf(__VA_ARGS__)
#define LOGF(TAG, ...) printf(__VA_ARGS__)
#endif

#endif /* CROSSPLATFORM_H_ */
