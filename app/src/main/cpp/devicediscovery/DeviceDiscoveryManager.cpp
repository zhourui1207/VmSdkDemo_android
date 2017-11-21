//
// Created by zhou rui on 2017/11/10.
//

#include "DeviceDiscoveryManager.h"
#include "packet/ObcpPacket.h"

namespace Dream {

    DeviceDiscoveryHandler::DeviceDiscoveryHandler(LooperPtr looperPtr,
                                                   std::shared_ptr<DeviceDiscoveryManager> managerSp)
            : Handler(looperPtr), _managerWp(managerSp) {

    }

    DeviceDiscoveryHandler::~DeviceDiscoveryHandler() {

    }

    void DeviceDiscoveryHandler::handleMessage(MessagePtr msgPtr) {
        auto managerWp = _managerWp.lock();
        if (managerWp.get()) {
            switch (msgPtr->_what) {
                case DeviceDiscoveryManager::MSG_TYPE_SEND_PACKET: {
                    managerWp->sendDeviceDiscoveryPacket();
                    int32_t intervalSec = msgPtr->_arg1;
                    MessagePtr newMsgPtr = Message::obtain(msgPtr);
                    sendMessageDelayed(newMsgPtr, intervalSec * 1000);
                    break;
                }
                case DeviceDiscoveryManager::MSG_TYPE_CLEARUP: {
                    managerWp->clearSet();
                    break;
                }
                case DeviceDiscoveryManager::MSG_TYPE_FIND_DEVICE: {
                    managerWp->findDevice(msgPtr);
                    break;
                }
                default:
                    break;
            }
        }
    }

    DeviceDiscoveryManager::DeviceDiscoveryManager()
            : _intervalSec(INTERVAL_SEC_DEFAULT), _deviceFindCallBack(nullptr), _sendCount(0) {

    }

    DeviceDiscoveryManager::~DeviceDiscoveryManager() {

    }

    bool DeviceDiscoveryManager::start(fDeviceFindCallBack deviceFindCallBack) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_handlerThreadUp.get()) {
            LOGE(TAG, "Already starting!\n");
            return false;
        }

        LOGD(TAG, "Send packet begin! device find call back [%zd]\n", deviceFindCallBack);

        _deviceFindCallBack = deviceFindCallBack;

        _udpServerUp.reset(new AsioUdpServer(
                std::bind(&DeviceDiscoveryManager::receiveData, this, std::placeholders::_1,
                          std::placeholders::_2, std::placeholders::_3, std::placeholders::_4),
                "", DEVICE_DISCOVERY_PORT));
        _udpServerUp->start_up(DEVICE_DISCOVERY_ADDR);

        _handlerThreadUp.reset(new HandlerThread());
        _handlerThreadUp->start();

        _handlerSp.reset(
                new DeviceDiscoveryHandler(_handlerThreadUp->getLooper(), shared_from_this()));

        MessagePtr msgPtr = _handlerSp->obtainMessage(MSG_TYPE_SEND_PACKET, _intervalSec, 0);
        _handlerSp->sendMessageDelayed(msgPtr, 200);

        return true;
    }

    void DeviceDiscoveryManager::stop() {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_handlerThreadUp.get()) {
            LOGD(TAG, "Send packet stop!\n");
            _handlerThreadUp->quit();
            _handlerThreadUp.reset();
        }
        if (_handlerSp.get()) {
            _handlerSp.reset();
        }
        if (_udpServerUp.get()) {
            _udpServerUp->shut_down();
            _udpServerUp.reset();
        }
    }

    void DeviceDiscoveryManager::clearup() {
        std::lock_guard<std::mutex> lock(_mutex);
        LOGD(TAG, "Clearup!\n");
        if (_handlerThreadUp.get() && _handlerSp.get()) {
            _handlerSp->sendEmptyMessage(MSG_TYPE_CLEARUP);
        } else {
            clearSet();
        }
    }

    void DeviceDiscoveryManager::setAutoRequestInterval(int32_t intervalSec) {
        std::lock_guard<std::mutex> lock(_mutex);
        LOGD(TAG, "Set auto request interval [%d]!\n", intervalSec);
        _intervalSec = intervalSec;
    }

    void DeviceDiscoveryManager::receiveData(const std::string &remote_address,
                                             unsigned short remote_port, const char *pBuf,
                                             std::size_t len) {
        if (len < ObcpPacket::HEADER_HENGHT + 156) {  // 兼容老版本
            LOGE(TAG, "The packet's length is too small, len [%d]", len);
            return;
        }
//        LOGW(TAG, "Receive data, len = [%d], [%s]\n", len, getHexString(pBuf, 0, len).c_str());
        // 丢到线程中去处理，维持线程安全
        if (_handlerSp.get()) {
            ObjectPtr objectPtr;
            objectPtr.reset(new UdpData(remote_address, remote_port, len, pBuf));
            MessagePtr msgPtr = _handlerSp->obtainMessage(MSG_TYPE_FIND_DEVICE, objectPtr);
            _handlerSp->sendMessage(msgPtr);
        }
    }

    void DeviceDiscoveryManager::sendDeviceDiscoveryPacket() {
        if (_udpServerUp.get()) {
            char sendBuf[2048];
            ObcpPacket packet(ObcpPacket::COMMAND_DEVICE_DISCOVERY);
            int packetLen = packet.encode(sendBuf, 2048);
//            LOGW(TAG, "Send device discovery packet, packet len [%d] data [%s]\n", packetLen,
//                 getHexString(sendBuf, 0, packetLen).c_str());
            if (_udpServerUp->send(DEVICE_DISCOVERY_ADDR, DEVICE_DISCOVERY_PORT, sendBuf,
                                   (size_t) packetLen)) {
                ++_sendCount;
            }

            // 测试
//            JWDeviceInfo deviceInfo;
//            memset(&deviceInfo, 0, sizeof(deviceInfo));
//            int32_t random = rangeRand(0, 7, ((uint32_t) time(NULL)));
//            sprintf(deviceInfo.serial, "000000%02d", random);
//            ObcpPacket obcpPacket(0, 0, (const char *) &deviceInfo, sizeof(deviceInfo));
//            char tmpBuf[2048];
//            int len = obcpPacket.encode(tmpBuf, 2048);
//            receiveData("", 0, tmpBuf, (size_t) len);
        }
    }

    void DeviceDiscoveryManager::clearSet() {
        _foundDeviceSet.clear();
    }

    void DeviceDiscoveryManager::findDevice(MessagePtr msgPtr) {
        std::shared_ptr<UdpData> udpDataPtr = std::dynamic_pointer_cast<UdpData>(msgPtr->_objPtr);
        if (udpDataPtr.get()) {
            ObcpPacket obcpPacket;
            if (obcpPacket.decode(udpDataPtr->data(), udpDataPtr->length()) > 0) {
                JWDeviceInfo deviceInfo;
                memset(&deviceInfo, 0, sizeof(deviceInfo));
                memcpy(&deviceInfo, obcpPacket.data(), obcpPacket.dataLen());
                // 查询是否已经被搜索到过
                std::string serialStr = deviceInfo.serial;
                auto serialIt = _foundDeviceSet.find(serialStr);
                if (serialIt == _foundDeviceSet.end()) {  // 没有搜索到过
                    LOGW(TAG, "Found device, serial is [%s]\n", serialStr.c_str());
                    _foundDeviceSet.emplace(serialStr);
                    if (_deviceFindCallBack) {
                        _deviceFindCallBack(deviceInfo);
                    }
                }
            }
        }
    }
}