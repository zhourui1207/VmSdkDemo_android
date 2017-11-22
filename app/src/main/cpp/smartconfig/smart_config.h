//
// Created by zhou rui on 2017/11/9.
//

#ifndef DREAM_SMART_CONFIG_H
#define DREAM_SMART_CONFIG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

///加密的类型
typedef enum {
    ENCRY_NONE = 0x1,
    ENCRY_WEP_OPEN,
    ENCRY_WEP_SHARED,
    ENCRY_WPA_TKIP,
    ENCRY_WPA_AES,
} ENCRY_TYPE;

//在WEP加密模式下的key索引，因为WEP模式一般可以设置4个Key的，需要给出索引值
typedef enum {
    WEP_KEY_INDEX_0,
    WEP_KEY_INDEX_1,
    WEP_KEY_INDEX_2,
    WEP_KEY_INDEX_3,
    WEP_KEY_INDEX_MAX
} WEP_KEY_INEX_E;

///WEP的密码长度
typedef enum {
    WEP_PWD_LEN_64,
    WEP_PWD_LEN_128
} WEP_PWD_LEN_E;

//WEP的加密模式
typedef enum {
    WEP_ENC_OPEN,
    WEP_ENC_SHARED
} WEP_ENC_TYPE_E;


bool JWSmart_config(const char *wifiSsid, const char *wifiPwd, int64_t packetIntervalMillis = 0,
                    int64_t waitIntervalMillis = 100, int32_t encryType = ENCRY_WPA_TKIP,
                    int32_t keyIndex = WEP_KEY_INDEX_0, int32_t wepPwdLen = WEP_PWD_LEN_64);

void JWSmart_stop();

#ifdef __cplusplus
}
#endif

#endif //VMSDKDEMO_ANDROID_SMART_CONFIG_H
