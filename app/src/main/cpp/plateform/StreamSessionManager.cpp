/*
 * StreamSessionManager.cpp
 *
 *  Created on: 2016年10月19日
 *      Author: zhourui
 */

#include "StreamSessionManager.h"
#include "../rtp/RtpPacket.h"

#ifdef _ANDROID

extern JavaVM *g_pJavaVM;  // 定义外部变量，该变量在VmNet-lib.cpp中被定义和赋值
#endif

namespace Dream {

    StreamSession::StreamSession(unsigned streamId, const std::string &addr,
                                 unsigned short port, fStreamCallBack streamCallback,
                                 fStreamConnectStatusCallBack streamConnectStatusCallback,
                                 void *user, const std::string &monitorId,
                                 const std::string &deviceId,
                                 int playType, int clientType, bool rtp) : _streamId(streamId),
                                                                           _streamClient(addr, port,
                                                                                         std::bind(
                                                                                                 &StreamSession::onStream,
                                                                                                 this,
                                                                                                 std::placeholders::_1),
                                                                                         monitorId,
                                                                                         deviceId,
                                                                                         playType,
                                                                                         clientType),
                                                                           _streamCallback(
                                                                                   streamCallback),
                                                                           _streamCallbackExt(
                                                                                   nullptr),
                                                                           _streamConnectStatusCallback(
                                                                                   streamConnectStatusCallback),
                                                                           _user(user), _rtp(rtp) {
        _streamClient.setConnectStatusListener(
                std::bind(&StreamSession::onConnectStatus, this, std::placeholders::_1));
    }

    StreamSession::StreamSession(unsigned streamId, const std::string &addr, unsigned short port,
                                 fStreamCallBackExt streamCallback,
                                 fStreamConnectStatusCallBack streamConnectStatusCallback,
                                 void *user,
                                 const std::string &monitorId, const std::string &deviceId,
                                 int playType, int clientType, bool rtp)
            : _streamId(streamId),
              _streamClient(addr, port,
                            std::bind(&StreamSession::onStream,
                                      this, std::placeholders::_1), monitorId, deviceId, playType,
                            clientType),
              _streamCallback(nullptr),
              _streamCallbackExt(streamCallback),
              _streamConnectStatusCallback(
                      streamConnectStatusCallback),
              _user(user), _rtp(rtp) {
        _streamClient.setConnectStatusListener(
                std::bind(&StreamSession::onConnectStatus, this, std::placeholders::_1));
    }

    StreamSession::~StreamSession() {
//  _streamClient.cancelConnectStatusListener(); // 将这段注释掉，可以获取到stream断开的消息
    }

    bool StreamSession::startUp() {
        std::lock_guard<std::mutex> lock(_mutex);
        return _streamClient.startUp();
    }

    void StreamSession::shutDown() {
        std::lock_guard<std::mutex> lock(_mutex);
        _streamClient.shutDown();
#ifdef _ANDROID
        if (g_pJavaVM && _user) {
            JNIEnv *jniEnv = nullptr;
            if (g_pJavaVM->GetEnv(reinterpret_cast<void **>(&jniEnv), JNI_VERSION_1_6) == JNI_OK) {
                if (jniEnv) {
                    try {
                        jniEnv->DeleteGlobalRef(static_cast<jobject>(_user));
                        __android_log_print(ANDROID_LOG_WARN, "StreamSession",
                                            "删除android线程[%zd]的全局引用！[%zd]", jniEnv, _user);
                        _user = nullptr;
                    } catch (...) {

                    }
                }
            }
        }
#endif
        _streamClient.cancelConnectStatusListener();
        _streamClient.cancelStreamListener();
    }

    void StreamSession::onStream(const std::shared_ptr<StreamData> &streamDataPtr) {
        if (_streamCallback) {
//            LOGE("StreamSessionManager", "_streamCallback \n");
            // 去掉rtp头后再回调
            if (_rtp) {
                auto rtpPacketPtr = std::unique_ptr<RtpPacket>(new RtpPacket);
                if (rtpPacketPtr->Parse(streamDataPtr->data(), streamDataPtr->length()) == 0) {
                    // PS流交给上层应用处理
                    _streamCallback(_streamId, streamDataPtr->streamType(),
                                    rtpPacketPtr->GetPayloadType(), rtpPacketPtr->GetPayloadData(),
                                    rtpPacketPtr->GetPayloadLength(), rtpPacketPtr->GetTimestamp(),
                                    rtpPacketPtr->GetSequenceNumber(), rtpPacketPtr->HasMarker(),
                                    _user);
                } else {
                    LOGE("StreamSessionManager", "rtp 解析失败\n");
                }
            } else {
                _streamCallback(_streamId, 0, 0, streamDataPtr->data(), streamDataPtr->length(), 0,
                                0, 0, _user);
            }
        }

        if (_streamCallbackExt) {
//            LOGE("StreamSessionManager", "_streamCallbackExt\n");
            // 去掉rtp头后再回调
            if (_rtp) {
                auto rtpPacketPtr = std::unique_ptr<RtpPacket>(new RtpPacket);
                if (rtpPacketPtr->Parse(streamDataPtr->data(), streamDataPtr->length()) == 0) {
                    // PS流交给上层应用处理
                    _streamCallbackExt(_streamId, streamDataPtr->streamType(),
                                       rtpPacketPtr->GetPayloadType(),
                                       rtpPacketPtr->GetPayloadData(),
                                       rtpPacketPtr->GetPayloadLength(),
                                       rtpPacketPtr->GetTimestamp(),
                                       rtpPacketPtr->GetSequenceNumber(), rtpPacketPtr->HasMarker(),
                                       rtpPacketPtr->m_bJWExtHeader,
                                       rtpPacketPtr->m_isFirstFrame == 1,
                                       rtpPacketPtr->m_isLastFrame == 1,
                                       rtpPacketPtr->m_utcTimeStamp, _user);
                } else {
                    LOGE("StreamSessionManager", "rtp 解析失败\n");
                }
            } else {
                _streamCallbackExt(_streamId, 0, 0, streamDataPtr->data(), streamDataPtr->length(),
                                   0, 0, 0, false, false, false, 0, _user);
            }
        }
    }

    void StreamSession::onConnectStatus(bool isConnected) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_streamConnectStatusCallback) {
//            LOGE("StreamSessionManager", "onConnectStatus start\n");
            _streamConnectStatusCallback(_streamId, isConnected, _user);
//            LOGE("StreamSessionManager", "onConnectStatus end\n");
        }
    }

    StreamSessionManager::StreamSessionManager() :
            _streamId(0) {
    }

    StreamSessionManager::~StreamSessionManager() {

    }

    bool StreamSessionManager::addStreamClient(const std::string &addr,
                                               unsigned short port, fStreamCallBack streamCallback,
                                               fStreamConnectStatusCallBack streamConnectStatusCallback,
                                               void *pUser, unsigned &streamId,
                                               const std::string &monitorId,
                                               const std::string &deviceId,
                                               int playType, int clientType, bool rtp) {
        std::lock_guard<std::mutex> lock(_mutex);

        if (!getStreamId(streamId)) {  // 连接数到上限
            return false;
        }

        auto streamSessionPtr = std::make_shared<StreamSession>(streamId, addr, port,
                                                                streamCallback,
                                                                streamConnectStatusCallback, pUser,
                                                                monitorId, deviceId, playType,
                                                                clientType, rtp);

        if (!streamSessionPtr->startUp()) {
            recoveryStreamId(streamId);
            return false;
        }
        _sessionMap.insert(std::make_pair(streamId, streamSessionPtr));
        return true;
    }

    bool StreamSessionManager::addStreamClient(const std::string &addr, unsigned short port,
                                               fStreamCallBackExt streamCallback,
                                               fStreamConnectStatusCallBack streamConnectStatusCallback,
                                               void *pUser,
                                               unsigned &streamId, const std::string &monitorId,
                                               const std::string &deviceId,
                                               int playType, int clientType, bool rtp) {
        std::lock_guard<std::mutex> lock(_mutex);

        if (!getStreamId(streamId)) {  // 连接数到上限
            return false;
        }

        auto streamSessionPtr = std::make_shared<StreamSession>(streamId, addr, port,
                                                                streamCallback,
                                                                streamConnectStatusCallback, pUser,
                                                                monitorId, deviceId, playType,
                                                                clientType, rtp);

        if (!streamSessionPtr->startUp()) {
            recoveryStreamId(streamId);
            return false;
        }
        _sessionMap.insert(std::make_pair(streamId, streamSessionPtr));
        return true;
    }

    // 移除码流客户端
    void StreamSessionManager::removeStreamClient(unsigned streamId) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto streamSessionPtrIt = _sessionMap.find(streamId);
        if (streamSessionPtrIt != _sessionMap.end()) {
            streamSessionPtrIt->second->shutDown();
            _sessionMap.erase(streamSessionPtrIt);
            recoveryStreamId(streamId);
        }
    }

    bool StreamSessionManager::addRTSPStreamClient(const std::string &url,
                                                   fStreamCallBackV2 streamCallback,
                                                   fStreamConnectStatusCallBackV2 streamConnectStatusCallback,
                                                   void *pUser, unsigned &streamId, bool encrypt) {
        std::lock_guard<std::mutex> lock(_mutex);

        if (!getStreamId(streamId)) {  // 连接数到上限
            return false;
        }

        auto rtspStreamSessionPtr = std::make_shared<RtspTcpClient>(url, encrypt);

        if (!rtspStreamSessionPtr->startUp()) {
            recoveryStreamId(streamId);
            return false;
        }
        rtspStreamSessionPtr->setStreamCallback(streamCallback);
        rtspStreamSessionPtr->setConnectStatusListener(streamConnectStatusCallback);
        rtspStreamSessionPtr->setUser(pUser);
        _rtspSessionMap.insert(std::make_pair(streamId, rtspStreamSessionPtr));
        return true;
    }

    void StreamSessionManager::removeRTSPStreamClient(unsigned streamId) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto rtspStreamSessionPtrIt = _rtspSessionMap.find(streamId);
        if (rtspStreamSessionPtrIt != _rtspSessionMap.end()) {
            rtspStreamSessionPtrIt->second->shutdown();
            _rtspSessionMap.erase(rtspStreamSessionPtrIt);
            recoveryStreamId(streamId);
        }
    }

    bool StreamSessionManager::pauseRTSPStream(unsigned streamId) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto rtspStreamSessionPtrIt = _rtspSessionMap.find(streamId);
        if (rtspStreamSessionPtrIt != _rtspSessionMap.end()) {
            rtspStreamSessionPtrIt->second->pause();
            return true;
        }
        return false;
    }

    bool StreamSessionManager::playRTSPStream(unsigned streamId, float scale) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto rtspStreamSessionPtrIt = _rtspSessionMap.find(streamId);
        if (rtspStreamSessionPtrIt != _rtspSessionMap.end()) {
            rtspStreamSessionPtrIt->second->play(scale);
            return true;
        }
        return false;
    }

    bool StreamSessionManager::streamIsValid(unsigned int streamId) {
        std::lock_guard<std::mutex> lock(_mutex);

        auto streamSessionPtrIt = _sessionMap.find(streamId);
        if (streamSessionPtrIt != _sessionMap.end()) {
            return true;
        }

        auto rtspStreamSessionPtrIt = _rtspSessionMap.find(streamId);
        return rtspStreamSessionPtrIt != _rtspSessionMap.end();

    }

// 清除所有码流连接
    void StreamSessionManager::clearAll() {
        std::lock_guard<std::mutex> lock(_mutex);

        for (auto streamSessionPtrIt = _sessionMap.begin();
             streamSessionPtrIt != _sessionMap.end(); ++streamSessionPtrIt) {
            streamSessionPtrIt->second->shutDown();
        }
        _sessionMap.clear();

        for (auto rtspStreamSessionPtrIt = _rtspSessionMap.begin();
             rtspStreamSessionPtrIt != _rtspSessionMap.end(); ++rtspStreamSessionPtrIt) {
            rtspStreamSessionPtrIt->second->shutdown();
        }
        _rtspSessionMap.clear();

        _streamId = 0;
        _recoveryStreamIds.clear();
    }

// 获得streamid 从 1 - uint_max
    bool StreamSessionManager::getStreamId(unsigned &streamId) {
        if (_recoveryStreamIds.size() > 0) {  // 如果有回收的，先用回收的
            streamId = *(_recoveryStreamIds.begin());
            _recoveryStreamIds.erase(streamId);
            return true;
        } else if (_streamId < UINT_MAX) {
            streamId = ++_streamId;
            return true;
        } else {  // id分配完了
            return false;
        }
    }

// 还回streamid
    void StreamSessionManager::recoveryStreamId(int streamId) {
        _recoveryStreamIds.emplace(streamId);
    }

} /* namespace Dream */
