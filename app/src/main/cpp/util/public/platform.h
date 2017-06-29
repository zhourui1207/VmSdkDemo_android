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

inline time_t getCurrentTimeStamp() {
    std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> tp = std::chrono::time_point_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now());
    auto tmp = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());
    return (time_t) tmp.count();
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
