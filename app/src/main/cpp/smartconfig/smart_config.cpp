//
// Created by zhou rui on 2017/11/9.
//

#include "smart_config.h"
#include "WifiConfig.h"

Dream::WifiConfig *g_pWifiConfig = nullptr;

bool config(const char *wifiSsid, const char *wifiPwd, int64_t packetIntervalMillis,
            int64_t waitIntervalMillis, int32_t encryType, int32_t keyIndex, int32_t wepPwdLen) {
    if (!g_pWifiConfig) {
        g_pWifiConfig = new Dream::WifiConfig();
    }
    if (g_pWifiConfig) {
        return g_pWifiConfig->config(wifiSsid, wifiPwd, packetIntervalMillis, waitIntervalMillis,
                                     encryType, keyIndex, wepPwdLen);
    }
    return false;
}

void stop() {
    if (g_pWifiConfig) {
        g_pWifiConfig->stop();
    }
}