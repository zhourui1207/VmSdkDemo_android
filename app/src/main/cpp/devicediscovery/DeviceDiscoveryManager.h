//
// Created by zhou rui on 2017/11/10.
//

#ifndef DREAM_DEVICEDISCOVERYMANAGER_H
#define DREAM_DEVICEDISCOVERYMANAGER_H


#include <set>
#include "device_discovery.h"
#include "../util/HandlerThread.h"
#include "../util/AsioUdpServer.h"

namespace Dream {

    class DeviceDiscoveryManager;

    class DeviceDiscoveryHandler : public Handler {
    public:
        DeviceDiscoveryHandler(LooperPtr looperPtr,
                               std::shared_ptr<DeviceDiscoveryManager> managerWp);

        virtual ~DeviceDiscoveryHandler();

        virtual void handleMessage(MessagePtr msgPtr) override ;

    private:
        std::weak_ptr<DeviceDiscoveryManager> _managerWp;
    };

    class DeviceDiscoveryManager : public std::enable_shared_from_this<DeviceDiscoveryManager> {
    public:
        friend class DeviceDiscoveryHandler;

        const static int32_t INTERVAL_SEC_DEFAULT = 5; // 默认5秒发送一次请求

    private:
        const static int MSG_TYPE_SEND_PACKET = 1;
        const static int MSG_TYPE_CLEARUP = 2;
        const static int MSG_TYPE_FIND_DEVICE = 3;

        const char* DEVICE_DISCOVERY_ADDR = "228.228.228.228";
        static const unsigned short DEVICE_DISCOVERY_PORT = 3721;
        const char *TAG = "DeviceDiscoveryMgr";

    public:
        DeviceDiscoveryManager();

        virtual ~DeviceDiscoveryManager();

        bool start(fDeviceFindCallBack deviceFindCallBack);

        void stop();

        void clearup();

        // 设置请求间隔，必须再start前设置才生效
        void setAutoRequestInterval(int32_t intervalSec);

    private:
        void
        receiveData(const std::string &remote_address, unsigned short remote_port, const char *pBuf,
                    std::size_t len);

        void sendDeviceDiscoveryPacket();

        void clearSet();

        void findDevice(MessagePtr msgPtr);

        std::mutex _mutex;
        std::unique_ptr<HandlerThread> _handlerThreadUp;
        HandlerPtr _handlerSp;
        int32_t _intervalSec;  // 请求发送间隔
        fDeviceFindCallBack _deviceFindCallBack;  // 设备被找到回调
        std::unique_ptr<AsioUdpServer> _udpServerUp;
        std::set<std::string> _foundDeviceSet;  // 已发现的设备集合
        uint32_t _sendCount;
    };

}

#endif //VMSDKDEMO_ANDROID_DEVICEDISCOVERYMANAGER_H
