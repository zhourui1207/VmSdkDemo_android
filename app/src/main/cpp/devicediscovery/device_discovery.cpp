//
// Created by zhou rui on 2017/11/10.
//

#include <memory>
#include "device_discovery.h"
#include "DeviceDiscoveryManager.h"

std::shared_ptr<Dream::DeviceDiscoveryManager> g_deviceDiscoveryMgrSp;

bool JWDeviceDiscovery_init() {
    if (!g_deviceDiscoveryMgrSp.get()) {
        g_deviceDiscoveryMgrSp.reset(new Dream::DeviceDiscoveryManager());
    }
    return g_deviceDiscoveryMgrSp.get() != nullptr;
}

void JWDeviceDiscovery_uninit() {
    if (g_deviceDiscoveryMgrSp.get()) {
        g_deviceDiscoveryMgrSp->stop();
        g_deviceDiscoveryMgrSp.reset();
    }
}

bool JWDeviceDiscovery_start(fJWDeviceFindCallBack deviceFindCallBack) {
    if (g_deviceDiscoveryMgrSp.get()) {
        return g_deviceDiscoveryMgrSp->start(deviceFindCallBack);
    }
    return false;
}

void JWDeviceDiscovery_stop() {
    if (g_deviceDiscoveryMgrSp.get()) {
        g_deviceDiscoveryMgrSp->stop();
    }
}

void JWDeviceDiscovery_clearup() {
    if (g_deviceDiscoveryMgrSp.get()) {
        g_deviceDiscoveryMgrSp->clearup();
    }
}

// 设置请求间隔，必须再start前设置才生效
void JWDeviceDiscovery_setAutoRequestInterval(int32_t intervalSec) {
    if (g_deviceDiscoveryMgrSp.get()) {
        g_deviceDiscoveryMgrSp->setAutoRequestInterval(intervalSec);
    }
}