/*
 * MainModule.cpp
 *
 *  Created on: 2016年10月18日
 *      Author: zhourui
 */

#include "ErrorCode.h"
#include "MainModule.h"

namespace Dream {

    unsigned MainModule::connect(const std::string &serverAddr, unsigned port) {
        if (!startUasClient(serverAddr, port)) {
            // sdk启动失败
            return ERR_CODE_SDK_STARTUP_FAILED;
        }
        return ERR_CODE_OK;
    }

    void MainModule::disconnect() {
        stopUasClient();
    }

    unsigned MainModule::login(const std::string &loginName,
                               const std::string &loginPwd) {
        if (!_uasConnected.load() || _uasClientPtr.get() == nullptr) {
            return ERR_CODE_NO_CONNECT;
        }
        return _uasClientPtr->login(loginName, loginPwd);
    }

    void MainModule::logout() {
        if (!_uasConnected.load() || _uasClientPtr.get() == nullptr) {
            return;
        }
        _uasClientPtr->logout();
    }

    unsigned MainModule::getDepTrees(DepTreeList &depTrees) {
        if (!_uasConnected.load() || _uasClientPtr.get() == nullptr) {
            return ERR_CODE_NO_CONNECT;
        }
        return _uasClientPtr->getDepTrees(depTrees);
    }

    unsigned MainModule::getChannels(int depId, ChannelList &channels) {
        if (!_uasConnected.load() || _uasClientPtr.get() == nullptr) {
            return ERR_CODE_NO_CONNECT;
        }
        return _uasClientPtr->getChannels(depId, channels);
    }

    unsigned MainModule::getRecords(const std::string &fdId, int channelId,
                                    unsigned beginTime, unsigned endTime, bool isCenter,
                                    RecordList &records) {
        if (!_uasConnected.load() || _uasClientPtr.get() == nullptr) {
            return ERR_CODE_NO_CONNECT;
        }
        return _uasClientPtr->getRecords(fdId, channelId, beginTime, endTime,
                                         isCenter, records);
    }

    unsigned MainModule::getAlarms(const std::string &fdId, int channelId,
                                   int channelBigType, const std::string &beginTime,
                                   const std::string &endTime, AlarmList &alarms) {
        if (!_uasConnected.load() || _uasClientPtr.get() == nullptr) {
            return ERR_CODE_NO_CONNECT;
        }
        return _uasClientPtr->getAlarms(fdId, channelId, channelBigType, beginTime,
                                        endTime, alarms);
    }

    unsigned MainModule::openRealplayStream(const std::string &fdId, int channelId,
                                            bool isSub, unsigned &monitorId, std::string &videoAddr,
                                            unsigned &videoPort,
                                            std::string &audioAddr, unsigned &audioPort) {
        if (!_uasConnected.load() || _uasClientPtr.get() == nullptr) {
            return ERR_CODE_NO_CONNECT;
        }
        return _uasClientPtr->openRealplayStream(fdId, channelId, isSub, monitorId,
                                                 videoAddr, videoPort, audioAddr, audioPort);
    }

    void MainModule::closeRealplayStream(unsigned monitorId) {
        if (!_uasConnected.load() || _uasClientPtr.get() == nullptr) {
            return;
        }
        _uasClientPtr->closeRealplayStream(monitorId);
    }

    unsigned MainModule::openPlaybackStream(const std::string &fdId, int channelId,
                                            bool isCenter, unsigned beginTime, unsigned endTime,
                                            unsigned &monitorId,
                                            std::string &videoAddr, unsigned &videoPort,
                                            std::string &audioAddr,
                                            unsigned &audioPort) {
        if (!_uasConnected.load() || _uasClientPtr.get() == nullptr) {
            return ERR_CODE_NO_CONNECT;
        }
        return _uasClientPtr->openPlaybackStream(fdId, channelId, isCenter, beginTime,
                                                 endTime, monitorId, videoAddr, videoPort,
                                                 audioAddr, audioPort);
    }

    void MainModule::closePlaybackStream(unsigned monitorId) {
        if (!_uasConnected.load() || _uasClientPtr.get() == nullptr) {
            return;
        }
        _uasClientPtr->closePlaybackStream(monitorId);
    }

    unsigned MainModule::controlPlayback(unsigned monitorId,
                                         unsigned controlId, const std::string &action,
                                         const std::string &param) {
        if (!_uasConnected.load() || _uasClientPtr.get() == nullptr) {
            return ERR_CODE_NO_CONNECT;
        }
        return _uasClientPtr->controlPlayback(monitorId, controlId, action, param);
    }

    unsigned MainModule::startStream(const std::string &addr, unsigned short port,
                                     fStreamCallBack streamCallback,
                                     fStreamConnectStatusCallBack streamConnectStatusCallback,
                                     void *pUser, unsigned &streamId,
                                     const std::string &monitorId, const std::string &deviceId,
                                     int playType, int clientType, bool rtp) {
        if (!_streamSessionManager.addStreamClient(addr, port, streamCallback,
                                                   streamConnectStatusCallback, pUser, streamId,
                                                   monitorId, deviceId, playType, clientType, rtp)) {
            return ERR_CODE_CREATE_STREAM_THREAD_FAILED;
        }
        return ERR_CODE_OK;
    }

    unsigned MainModule::startStream(const std::string &addr, unsigned short port,
                         fStreamCallBackExt streamCallback,
                         fStreamConnectStatusCallBack streamConnectStatusCallback, void *pUser,
                         unsigned &streamId, const std::string& monitorId,
                         const std::string &deviceId, int playType, int clientType,
                         bool rtp) {
        if (!_streamSessionManager.addStreamClient(addr, port, streamCallback,
                                                   streamConnectStatusCallback, pUser, streamId,
                                                   monitorId, deviceId, playType, clientType, rtp)) {
            return ERR_CODE_CREATE_STREAM_THREAD_FAILED;
        }
        return ERR_CODE_OK;
    }

    void MainModule::stopStream(unsigned streamId) {
//  if (!_uasConnected.load() || _uasClientPtr.get() == nullptr) {
//    return;
//  }
        _streamSessionManager.removeStreamClient(streamId);
    }

    unsigned MainModule::startRtspStream(const std::string &url,
                                         fStreamCallBackV2 streamCallback,
                                         fStreamConnectStatusCallBackV2 streamConnectStatusCallback,
                                         void *pUser, unsigned &streamId, bool encrypt) {
        if (!_streamSessionManager.addRTSPStreamClient(url, streamCallback,
                                                   streamConnectStatusCallback, pUser, streamId,
                                                   encrypt)) {
            return ERR_CODE_CREATE_STREAM_THREAD_FAILED;
        }
        return ERR_CODE_OK;
    }

    // 停止获取码流
    void MainModule::stopRtspStream(unsigned streamId) {
        _streamSessionManager.removeRTSPStreamClient(streamId);
    }

    unsigned MainModule::pauseRtspStream(unsigned streamId) {
        if (!_streamSessionManager.pauseRTSPStream(streamId)) {
            return ERR_CODE_SESSION_NOEXIST;
        }
        return ERR_CODE_OK;
    }

    unsigned MainModule::playRtspStream(unsigned streamId) {
        if (!_streamSessionManager.playRTSPStream(streamId)) {
            return ERR_CODE_SESSION_NOEXIST;
        }
        return ERR_CODE_OK;
    }

    unsigned MainModule::speedRtspStream(unsigned streamId, float speed) {
        if (!_streamSessionManager.playRTSPStream(streamId, speed)) {
            return ERR_CODE_SESSION_NOEXIST;
        }
        return ERR_CODE_OK;
    }

    bool MainModule::streamIsValid(unsigned streamId) {
        return _streamSessionManager.streamIsValid(streamId);
    }

    void MainModule::clearAllStream() {
        _streamSessionManager.clearAll();
    }

    void MainModule::sendCmd(const std::string &fdId, int channelId,
                             unsigned controlType, unsigned param1, unsigned param2) {
        if (!_uasConnected.load() || _uasClientPtr.get() == nullptr) {
            return;
        }
        _uasClientPtr->sendCmd(fdId, channelId, controlType, param1, param2);
    }

    bool MainModule::startUasClient(const std::string &serverAddr, unsigned port) {
        // 如果存在uasclient，先关闭
        if (_uasClientPtr.get() != nullptr) {
            _uasClientPtr->shutDown();
        }

        // 重置uasclient智能指针，并启动
        _uasClientPtr.reset(new UasClient(_pool, serverAddr, port));

        _uasConnected.store(false);
        _uasClientPtr->setConnectStatusListener(
                std::bind(&MainModule::onUasConnectStatus, this, std::placeholders::_1));
        _uasClientPtr->setAlarmListener(
                std::bind(&MainModule::onRealAlarm, this, std::placeholders::_1,
                          std::placeholders::_2, std::placeholders::_3, std::placeholders::_4,
                          std::placeholders::_5));
        bool startUpOk = _uasClientPtr->startUp();

        if (!startUpOk) {
            _uasClientPtr.reset();
        }

        return startUpOk;
    }

    void MainModule::stopUasClient() {
        if (_uasClientPtr.get() != nullptr) {
            _uasClientPtr->cancelConnectStatusListener();
            _uasClientPtr->cancelAlarmListener();
            _uasClientPtr->shutDown();
            _uasConnected.store(false);
//            onUasConnectStatus(false);
        }
        _uasClientPtr.reset();
    }

    void MainModule::pauseUasClient() {
        if (_uasClientPtr.get() != nullptr) {
            _uasClientPtr->shutDown();
        }
    }

    bool MainModule::resumeUasClient() {
        if (_uasClientPtr.get() != nullptr) {
            return _uasClientPtr->startUp();
        }
        return true;
    }

// uas连接状态回调
    void MainModule::onUasConnectStatus(bool isConnected) {
        _uasConnected.store(isConnected);
        if (_uasConnectStatusCallback) {
            _uasConnectStatusCallback(isConnected);
        }
    }

    void MainModule::onRealAlarm(const std::string &fdId, int channelId,
                                 unsigned alarmType, unsigned param1, unsigned param2) {
        if (_realAlarmCallback) {
            _realAlarmCallback(fdId.c_str(), channelId, alarmType, param1, param2);
        }
    }

    bool MainModule::startTalk(fStreamCallBackV3 streamCallback, void *pUser) {
        if (_talkServer.get() != nullptr) {
            return true;
        }
        _talkServer.reset(new RtpUdpServer);
        _talkServer->setUser(pUser);
        _talkServer->setStreamCallback(streamCallback);
        if (!_talkServer->start_up()) {
            _talkServer.reset();
            return false;
        }
        return true;
    }

    bool MainModule::sendTalk(const std::string &remoteAddress, unsigned short remotePort,
                              const char *data, size_t dataLen) {
        if (_talkServer.get() != nullptr) {
            return _talkServer->send_packet(remoteAddress, remotePort, data, dataLen);
        }
        return false;
    }

    void MainModule::stopTalk() {
        if (_talkServer.get() != nullptr) {
            _talkServer->shut_down();
            _talkServer.reset();
        }
    }

    bool MainModule::startStreamHeartbeatServer() {
        if (_streamHeartbeatServer.get() != nullptr) {
            return true;
        }
        _streamHeartbeatServer.reset(new StreamHeartbeatServer);
        if (!_streamHeartbeatServer->start_up()) {
            _streamHeartbeatServer.reset();
            return false;
        }
        return true;
    }

    bool MainModule::sendHeartbeat(const std::string &remoteAddress, unsigned short remotePort,
                                   unsigned heartbeatType, const std::string &monitorId,
                                   const std::string &srcId) {
        if (_streamHeartbeatServer.get() != nullptr) {
            return _streamHeartbeatServer->send_packet(remoteAddress, remotePort, heartbeatType,
                                                       monitorId, srcId);
        }
        return false;
    }

    void MainModule::stopStreamHeartbeatServer() {
        if (_streamHeartbeatServer.get() != nullptr) {
            _streamHeartbeatServer->shut_down();
            _streamHeartbeatServer.reset();
        }
    }

} /* namespace Dream */
