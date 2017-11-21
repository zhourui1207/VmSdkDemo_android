//
// Created by zhou rui on 2017/11/10.
//

#include <memory>
#include "device_discovery.h"
#include "DeviceDiscoveryManager.h"

std::shared_ptr<Dream::DeviceDiscoveryManager> g_deviceDiscoveryMgrSp;

bool init() {
    if (!g_deviceDiscoveryMgrSp.get()) {
        g_deviceDiscoveryMgrSp.reset(new Dream::DeviceDiscoveryManager());
    }
    return g_deviceDiscoveryMgrSp.get() != nullptr;
}

void uninit() {
    if (g_deviceDiscoveryMgrSp.get()) {
        g_deviceDiscoveryMgrSp->stop();
        g_deviceDiscoveryMgrSp.reset();
    }
}

bool start(fDeviceFindCallBack deviceFindCallBack) {
    if (g_deviceDiscoveryMgrSp.get()) {
        return g_deviceDiscoveryMgrSp->start(deviceFindCallBack);
    }
    return false;
}

void stop() {
    if (g_deviceDiscoveryMgrSp.get()) {
        g_deviceDiscoveryMgrSp->stop();
    }
}

void clearup() {
    if (g_deviceDiscoveryMgrSp.get()) {
        g_deviceDiscoveryMgrSp->clearup();
    }
}

// 设置请求间隔，必须再start前设置才生效
void setAutoRequestInterval(int32_t intervalSec) {
    if (g_deviceDiscoveryMgrSp.get()) {
        g_deviceDiscoveryMgrSp->setAutoRequestInterval(intervalSec);
    }
}