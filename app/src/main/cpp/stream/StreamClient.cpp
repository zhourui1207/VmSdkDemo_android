/*
 * StreamClient.cpp
 *
 *  Created on: 2016年10月13日
 *      Author: zhourui
 */

#include "packet/ActivePacket.h"
#include "StreamClient.h"
#include "../cipher/base64encode.h"
#include "packet/AuthRespPacket.h"
#include "../cipher/md5.h"
#include "packet/AuthReqPacket.h"

namespace Dream {

    std::shared_ptr<StreamPacket> StreamClient::newPacketPtr(unsigned msgType) {
        std::shared_ptr<StreamPacket> packetPtr;
        bool isMatch = false;
        switch (msgType) {
            case MSG_STREAM_VIDEO:
                isMatch = true;
                break;
            case MSG_STREAM_AUDIO:
                isMatch = true;
                break;
            case MSG_AUTH_RESP:
                packetPtr.reset(new AuthRespPacket());
                break;
            default:
                LOGW("StreamClient", "未匹配到相应的包! msgType=[%d]\n", msgType);
                break;
        }
        if (isMatch) {
            packetPtr.reset(new StreamPacket(msgType));
        }
        return packetPtr;
    }

    void StreamClient::receive(const std::shared_ptr<StreamPacket> &packetPtr) {
//        LOGE("StreamClient", "receive start\n");
        if (!_streamCallback) {
//            LOGE("StreamClient", "receive end\n");
            return;
        }
        bool isStream = false;
        unsigned msgType = packetPtr->msgType();
        StreamData::STREAM_TYPE streamType = StreamData::STREAM_TYPE_VIDEO;
        switch (msgType) {
            case MSG_STREAM_VIDEO: {
                isStream = true;
                break;
            }
            case MSG_STREAM_AUDIO: {
                streamType = StreamData::STREAM_TYPE_AUDIO;
                isStream = true;
                break;
            }
            case MSG_AUTH_RESP: {  // 鉴定权限返回包
                LOGW("StreamClient", "receive auth resp!\n");
                if (!_auth && !_monitorId.empty() && !_deviceId.empty()) {
                    // 强转
                    AuthRespPacket* authPacket = (AuthRespPacket *) packetPtr.get();
                    if (authPacket) {
                        std::string extKey = "joywarecloud" + _monitorId + _deviceId;
                        MD5_CTX ctx;
                        MD5Init(&ctx);
                        MD5Update(&ctx, (const unsigned char *) extKey.c_str(), extKey.length());
                        unsigned char digest[16];
                        MD5Final(digest, &ctx);
                        char base64Key[25] = {0};
                        base64_encode(digest, 16, base64Key);
                        std::string finalKey = std::string(base64Key);
                        if (finalKey == authPacket->key()) {
                            LOGW("StreamClient", "auth success!\n");
                            _auth = true;
                        } else {
                            LOGW("StreamClient", "auth failed, server key[%s]\n", authPacket->key().c_str());
                        }
                    } else {
                        LOGE("StreamClient", "parse auth resp failed! server key[%s]\n", packetPtr->data());
                    }
                }
                break;
            }
            default: {
                LOGW("StreamClient", "非码流包!\n");
                break;
            }
        }
        if (isStream && _streamCallback) {
//            LOGE("StreamClient", "streamDataPtr start\n");
            auto streamDataPtr = std::make_shared<StreamData>(streamType, packetPtr->dataLen(),
                                                              packetPtr->data());
//            LOGE("StreamClient", "streamDataPtr end\n");

            _streamCallback(streamDataPtr);
//            LOGE("StreamClient", "_streamCallback end\n");
        }
//        LOGE("StreamClient", "receive end\n");
    }

    void StreamClient::onConnect() {
        auth();
        printf("已连接，开启心跳定时器\n");
        if (_timerPtr.get() != nullptr) {
            _timerPtr->cancel();
        }
        _timerPtr.reset(
                new Timer(std::bind(&StreamClient::heartbeat, this), 0, true, _heartbeatInterval));
        _timerPtr->start();
    }

    void StreamClient::onClose() {
        if (_timerPtr.get() != nullptr) {
            printf("已断开，取消心跳定时器\n");
            _timerPtr->cancel();
            _timerPtr.reset();
        }
        _auth = false;
    }

    void StreamClient::heartbeat() {
        ActivePacket req;
        send(req);
    }

    void StreamClient::auth() {
        if (!_monitorId.empty() && !_deviceId.empty()) {
            LOGW("StreamClient", "start auth\n");
            //播放类型;mointorId;客户端类型;base64(md5(monitorId+"joywarecloud" +deviceId)
            std::string extKey = _monitorId + "joywarecloud" + _deviceId;
            MD5_CTX ctx;
            MD5Init(&ctx);
            MD5Update(&ctx, (const unsigned char *) extKey.c_str(), extKey.length());
            unsigned char digest[16];
            MD5Final(digest, &ctx);
            char base64Key[25] = {0};
            base64_encode(digest, 16, base64Key);

            char playType[100] = {0};
            char clientType[100] = {0};
            sprintf(playType, "%d", _playType);
            sprintf(clientType, "%d", _clientType);
            std::string sendStr = std::string(playType) + ";" + _monitorId + ";" + std::string(clientType) + ";" + base64Key;
            AuthReqPacket req(sendStr);
            send(req);
        }
    }

} /* namespace Dream */
