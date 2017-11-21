/*
 * VmNet.cpp
 *
 *  Created on: 2016年10月14日
 *      Author: zhourui
 *      在这一层加入了gbk转utf－8功能，默认开启，若需要禁止转换，#define GBK_2_UTF8_DISABLE
 */

#include "../rtsp/RtspTcpClient.h"
#include "../util/public/StringUtil.h"
#include "ErrorCode.h"
#include "MainModule.h"
#include "VmNet.h"
#include "../rtp/RtpPacket.h"

#define GBK2UTF8_TMP_MAX_LEN 1024  // gbk转tmp时使用的临时变量最大长度

Dream::MainModule *g_pMainModule = nullptr;


// gbk转utf-8，若定义了GBK_2_UTF8_DISABLE，则该功能无效
int GBK2UTF8(const char *inbuf, size_t inlen, char *outbuf, size_t outlen) {
    // 如果输入长度大于输出长度，那么就要报错
    if (inlen > outlen) {
        return -1;
    }
#ifdef GBK_2_UTF8_DISABLE
    strncpy(outbuf, inbuf, outlen);
    return 0;
#else
    return g2u(inbuf, inlen, outbuf, outlen);
#endif
}

int GBK2UTF8_STR(const std::string &inStr, char *outbuf, size_t outlen) {
    return GBK2UTF8(inStr.c_str(), inStr.length(), outbuf, outlen);
}

VMNET_API bool CALL_METHOD VmNet_Init(unsigned uMaxThreadCount) {
    LOGD("VMSDK", "VmNet_Init\n");
    if (g_pMainModule == nullptr) {
        g_pMainModule = new Dream::MainModule(uMaxThreadCount);
        return true;
    }
    return false;
}

VMNET_API void CALL_METHOD VmNet_UnInit() {
    LOGD("VMSDK", "VmNet_UnInit\n");
    if (g_pMainModule != nullptr) {
        delete g_pMainModule;
        g_pMainModule = nullptr;
    }
}

// 开启对讲
VMNET_API bool CALL_METHOD VmNet_StartTalk(fStreamCallBackV3 streamCallback, void *pUser) {
    LOGD("VMSDK", "VmNet_StartTalk\n");
    if (g_pMainModule != nullptr) {
        return g_pMainModule->startTalk(streamCallback, pUser);
    }
    return false;
}

// 发送对讲数据
VMNET_API bool CALL_METHOD
VmNet_SendTalk(const char *sRemoteAddress, unsigned short usRemotePort, const char *oData,
               unsigned uDataLen) {
    if (g_pMainModule != nullptr) {
        return g_pMainModule->sendTalk(sRemoteAddress, usRemotePort, oData, uDataLen);
    }
    return false;
}

// 关闭对讲
VMNET_API void CALL_METHOD VmNet_StopTalk() {
    LOGD("VMSDK", "VmNet_StopTalk\n");
    if (g_pMainModule != nullptr) {
        g_pMainModule->stopTalk();
    }
}

// 开启心跳服务
VMNET_API bool CALL_METHOD VmNet_StartStreamHeartbeatServer() {
    LOGD("VMSDK", "VmNet_StartStreamHeartbeatServer\n");
    if (g_pMainModule != nullptr) {
        return g_pMainModule->startStreamHeartbeatServer();
    }
    return false;
}
// 发送心跳
VMNET_API bool CALL_METHOD
VmNet_SendHeartbeat(const char *sRemoteAddress, unsigned short usRemotePort,
                    unsigned uHeartbeatType,
                    const char *sMonitorId, const char *sSrcId) {
    if (g_pMainModule != nullptr) {
        return g_pMainModule->sendHeartbeat(sRemoteAddress, usRemotePort, uHeartbeatType,
                                            sMonitorId, sSrcId);
    }
    return false;
}

// 关闭心跳服务
VMNET_API void CALL_METHOD VmNet_StopStreamHeartbeatServer() {
    LOGD("VMSDK", "VmNet_StopStreamHeartbeatServer\n");
    if (g_pMainModule != nullptr) {
        g_pMainModule->stopStreamHeartbeatServer();
    }
}

VMNET_API unsigned CALL_METHOD VmNet_Connect(const char *sServerAddr,
                                             unsigned uServerPort,
                                             fUasConnectStatusCallBack cbUasConnectStatus) {
    LOGD("VMSDK", "VmNet_Connect\n");
    if (g_pMainModule == nullptr) {
        return ERR_CODE_SDK_UNINIT;
    }
    g_pMainModule->setUasConnectStatusCallback(cbUasConnectStatus);
    return g_pMainModule->connect(sServerAddr, uServerPort);
}

// 断开服务器
VMNET_API void CALL_METHOD VmNet_Disconnect() {
    LOGD("VMSDK", "VmNet_Disconnect\n");
    if (g_pMainModule == nullptr) {
        return;
    }
    g_pMainModule->cancelUasConnectStatusCallback();
    g_pMainModule->disconnect();
}

VMNET_API unsigned CALL_METHOD VmNet_Login(const char *sLoginName,
                                           const char *sLoginPwd) {
    LOGD("VMSDK", "VmNet_Login\n");
    if (g_pMainModule == nullptr) {
        return ERR_CODE_SDK_UNINIT;
    }
    return g_pMainModule->login(sLoginName, sLoginPwd);
}

VMNET_API void CALL_METHOD VmNet_Logout() {
    LOGD("VMSDK", "VmNet_Logout\n");
    if (g_pMainModule == nullptr) {
        return;
    }
    g_pMainModule->logout();
}

VMNET_API unsigned CALL_METHOD VmNet_GetDepTrees(unsigned uPageNo,
                                                 unsigned uPageSize, VMNET_OUT
                                                 TVmDepTree *pDepTrees, VMNET_OUT
                                                 unsigned &uSize) {
    LOGD("VMSDK", "VmNet_GetDepTrees\n");
    if (g_pMainModule == nullptr) {
        return ERR_CODE_SDK_UNINIT;
    }
    Dream::DepTreeList list;
    unsigned ret = g_pMainModule->getDepTrees(list);
    if (ret == ERR_CODE_OK && list.size() > 0) {
        unsigned i = 0;
        for (; i < uPageSize && i < list.size(); ++i) {
            pDepTrees[i].nDepId = list[i]._depId;
            pDepTrees[i].nParentId = list[i]._parentId;
            GBK2UTF8_STR(list[i]._depName, pDepTrees[i].sDepName, 64);
            pDepTrees[i].uOnlineChannelCounts = list[i]._onlineChannelCounts;
            pDepTrees[i].uOfflineChannelCounts = list[i]._offlineChannelCounts;
        }
        uSize = i;
    } else {
        uSize = 0;
    }

    return ret;
}

VMNET_API unsigned CALL_METHOD VmNet_GetChannels(unsigned uPageNo,
                                                 unsigned uPageSize, int nDepId, VMNET_OUT
                                                 TVmChannel *pChannels,
                                                 VMNET_OUT unsigned &uSize) {
    LOGD("VMSDK", "VmNet_GetChannels\n");
    if (g_pMainModule == nullptr) {
        return ERR_CODE_SDK_UNINIT;
    }
    Dream::ChannelList list;
    unsigned ret = g_pMainModule->getChannels(nDepId, list);
    if (ret == ERR_CODE_OK && list.size() > 0) {
        unsigned i = 0;
        for (; i < uPageSize && i < list.size(); ++i) {
            pChannels[i].nDepId = list[i]._depId;
            GBK2UTF8_STR(list[i]._fdId, pChannels[i].sFdId, 32);
            pChannels[i].nChannelId = list[i]._channelId;
            pChannels[i].uChannelType = list[i]._channelType;
            GBK2UTF8_STR(list[i]._channelName, pChannels[i].sChannelName, 64);
            pChannels[i].uIsOnLine = list[i]._isOnLine;
            pChannels[i].uVideoState = list[i]._videoState;
            pChannels[i].uChannelState = list[i]._channelState;
            pChannels[i].uRecordState = list[i]._recordState;
        }
        uSize = i;
    } else {
        uSize = 0;
    }

    return ret;
}

VMNET_API unsigned CALL_METHOD VmNet_GetRecords(unsigned uPageNo,
                                                unsigned uPageSize, const char *sFdId,
                                                int nChannelId, unsigned uBeginTime,
                                                unsigned uEndTime, bool bIsCenter, VMNET_OUT
                                                TVmRecord *pRecords,
                                                VMNET_OUT unsigned &uSize) {
    LOGD("VMSDK", "VmNet_GetRecords\n");
    if (g_pMainModule == nullptr) {
        return ERR_CODE_SDK_UNINIT;
    }
    Dream::RecordList list;
    unsigned ret = g_pMainModule->getRecords(sFdId, nChannelId, uBeginTime,
                                             uEndTime, bIsCenter, list);
    if (ret == ERR_CODE_OK && list.size() > 0) {
        unsigned i = 0;
        for (; i < uPageSize && i < list.size(); ++i) {
            pRecords[i].uBeginTime = list[i]._beginTime;
            pRecords[i].uEndTime = list[i]._endTime;
            GBK2UTF8_STR(list[i]._playbackUrl, pRecords[i].sPlaybackUrl, 256);
            GBK2UTF8_STR(list[i]._downloadUrl, pRecords[i].sDownloadUrl, 256);
        }
        uSize = i;
    } else {
        uSize = 0;
    }

    return ret;
}

VMNET_API unsigned CALL_METHOD VmNet_GetAlarms(unsigned uPageNo,
                                               unsigned uPageSize, const char *sFdId,
                                               int nChannelId,
                                               unsigned uChannelBigType, const char *sBeginTime,
                                               const char *sEndTime,
                                               VMNET_OUT TVmAlarm *pAlarms, VMNET_OUT
                                               unsigned &uSize) {
    LOGD("VMSDK", "VmNet_GetAlarms\n");
    if (g_pMainModule == nullptr) {
        return ERR_CODE_SDK_UNINIT;
    }
    Dream::AlarmList list;
    unsigned ret = g_pMainModule->getAlarms(sFdId, nChannelId, uChannelBigType,
                                            sBeginTime, sEndTime, list);
    if (ret == ERR_CODE_OK && list.size() > 0) {
        unsigned i = 0;
        for (; i < uPageSize && i < list.size(); ++i) {
            GBK2UTF8_STR(list[i]._alarmId, pAlarms[i].sAlarmId, 40);
            GBK2UTF8_STR(list[i]._fdId, pAlarms[i].sFdId, 32);
            GBK2UTF8_STR(list[i]._fdName, pAlarms[i].sFdName, 64);
            pAlarms[i].nChannelId = list[i]._channelId;
            GBK2UTF8_STR(list[i]._channelName, pAlarms[i].sChannelName, 64);
            pAlarms[i].uChannelBigType = list[i]._channelBigType;
            GBK2UTF8_STR(list[i]._alarmTime, pAlarms[i].sAlarmTime, 40);
            pAlarms[i].uAlarmCode = list[i]._alarmCode;
            GBK2UTF8_STR(list[i]._alarmId, pAlarms[i].sAlarmId, 40);
            GBK2UTF8_STR(list[i]._alarmName, pAlarms[i].sAlarmName, 64);
            GBK2UTF8_STR(list[i]._alarmSubName, pAlarms[i].sAlarmSubName, 64);
            pAlarms[i].uAlarmType = list[i]._alarmType;
            GBK2UTF8_STR(list[i]._photoUrl, pAlarms[i].sPhotoUrl, 256);
        }
        uSize = i;
    } else {
        uSize = 0;
    }

    return ret;
}

VMNET_API void CALL_METHOD VmNet_StartReceiveRealAlarm(
        fRealAlarmCallBack cbRealAlarm) {
    LOGD("VMSDK", "VmNet_StartReceiveRealAlarm\n");
    if (g_pMainModule == nullptr) {
        return;
    }
    g_pMainModule->startReceiveRealAlarm(cbRealAlarm);
}

VMNET_API void CALL_METHOD VmNet_StopReceiveRealAlarm() {
    LOGD("VMSDK", "VmNet_StopReceiveRealAlarm\n");
    if (g_pMainModule == nullptr) {
        return;
    }
    g_pMainModule->stopReceiveRealAlarm();
}

VMNET_API unsigned CALL_METHOD VmNet_OpenRealplayStream(const char *sFdId,
                                                        int nChannelId, bool bIsSub, VMNET_OUT
                                                        unsigned &uMonitorId, VMNET_OUT
                                                        char *sVideoAddr,
                                                        VMNET_OUT unsigned &uVideoPort, VMNET_OUT
                                                        char *sAudioAddr, VMNET_OUT
                                                        unsigned &uAudioPort) {
    LOGD("VMSDK", "VmNet_OpenRealplayStream\n");

    if (g_pMainModule == nullptr) {
        return ERR_CODE_SDK_UNINIT;
    }
    std::string videoAddr, audioAddr;
    unsigned ret = g_pMainModule->openRealplayStream(sFdId, nChannelId, bIsSub,
                                                     uMonitorId, videoAddr, uVideoPort, audioAddr,
                                                     uAudioPort);
    if (ret == ERR_CODE_OK) {
        strcpy(sVideoAddr, videoAddr.c_str());
        strcpy(sAudioAddr, audioAddr.c_str());
    }
    return ret;
}

VMNET_API void CALL_METHOD VmNet_CloseRealplayStream(unsigned uMonitorId) {
    LOGD("VMSDK", "VmNet_CloseRealplayStream\n");
    if (g_pMainModule == nullptr) {
        return;
    }
    g_pMainModule->closeRealplayStream(uMonitorId);
}

VMNET_API unsigned CALL_METHOD VmNet_OpenPlaybackStream(const char *sFdId,
                                                        int nChannelId, bool bIsCenter,
                                                        unsigned beginTime, unsigned endTime,
                                                        VMNET_OUT unsigned &uMonitorId, VMNET_OUT
                                                        char *sVideoAddr, VMNET_OUT
                                                        unsigned &uVideoPort,
                                                        VMNET_OUT char *sAudioAddr, VMNET_OUT
                                                        unsigned &uAudioPort) {
    LOGD("VMSDK", "VmNet_OpenPlaybackStream\n");
    if (g_pMainModule == nullptr) {
        return ERR_CODE_SDK_UNINIT;
    }
    std::string videoAddr, audioAddr;
    unsigned ret = g_pMainModule->openPlaybackStream(sFdId, nChannelId, bIsCenter,
                                                     beginTime, endTime, uMonitorId, videoAddr,
                                                     uVideoPort, audioAddr,
                                                     uAudioPort);
    if (ret == ERR_CODE_OK) {
        strcpy(sVideoAddr, videoAddr.c_str());
        strcpy(sAudioAddr, audioAddr.c_str());
    }
    return ret;
}

// 停止回放流
// 入参    uMonitorId：监控id，打开回放码流时获得； bIsCenter：是否为中心录像
VMNET_API void CALL_METHOD VmNet_ClosePlaybackStream(unsigned uMonitorId) {
    LOGD("VMSDK", "VmNet_ClosePlaybackStream\n");
    if (g_pMainModule == nullptr) {
        return;
    }
    g_pMainModule->closePlaybackStream(uMonitorId);
}

// 控制回放
// 入参    uMonitorId：监控id，打开回放码流时获得； bIsCenter：是否为中心录像
//        uControlId：控制id； sAction：操作； sParam：参数
VMNET_API unsigned CALL_METHOD VmNet_ControlPlayback(unsigned uMonitorId, unsigned uControlId,
                                                     const char *sAction, const char *sParam) {
    LOGD("VMSDK", "VmNet_ControlPlayback\n");
    if (g_pMainModule == nullptr) {
        return ERR_CODE_SDK_UNINIT;
    }
    return g_pMainModule->controlPlayback(uMonitorId, uControlId, sAction, sParam);
}

VMNET_API unsigned CALL_METHOD VmNet_StartStream(const char *sAddr,
                                                 unsigned short uPort, fStreamCallBack cbRealData,
                                                 fStreamConnectStatusCallBack cbConnectStatus,
                                                 void *pUser, VMNET_OUT unsigned &uStreamId,
                                                 const char *sMonitorId, const char* sDeviceId,
                                                 int playType, int clientType) {
    if (g_pMainModule == nullptr) {
        return ERR_CODE_SDK_UNINIT;
    }
    return g_pMainModule->startStream(sAddr == nullptr ? "" : sAddr, uPort, cbRealData,
                                      cbConnectStatus, pUser, uStreamId,
                                      sMonitorId == nullptr ? "" : sMonitorId, sDeviceId, playType,
                                      clientType, true);
}

VMNET_API unsigned CALL_METHOD VmNet_StartStreamExt(const char *sAddr,
                                                 unsigned short uPort, fStreamCallBackExt cbRealData,
                                                 fStreamConnectStatusCallBack cbConnectStatus,
                                                 void *pUser, VMNET_OUT unsigned &uStreamId,
                                                 const char *sMonitorId, const char *sDeviceId,
                                                 int playType, int clientType) {
    if (g_pMainModule == nullptr) {
        return ERR_CODE_SDK_UNINIT;
    }
    return g_pMainModule->startStream(sAddr == nullptr ? "" : sAddr, uPort, cbRealData,
                                      cbConnectStatus, pUser, uStreamId,
                                      sMonitorId == nullptr ? "" : sMonitorId, sDeviceId, playType,
                                      clientType, true);
}

VMNET_API void CALL_METHOD VmNet_StopStream(unsigned uStreamId) {
    LOGI("VMSDK", "VmNet_StopStream(%d)\n", uStreamId);
    if (g_pMainModule == nullptr) {
        return;
    }
    g_pMainModule->stopStream(uStreamId);
}

//VMNET_API bool CALL_METHOD
//VmNet_StartStreamByRtsp(const char *rtspUrl, fStreamCallBackV2 cbStreamData,
//                        fStreamConnectStatusCallBackV2 cbConnectStatus,
//                        void *pUser, VMNET_OUT long &rtspStreamHandle, bool encrypt) {
//    LOGD("VMSDK", "VmNet_StartStreamByRtsp\n");
//    Dream::RtspTcpClient *pRtspTcpClient = new Dream::RtspTcpClient(rtspUrl, encrypt);
//    pRtspTcpClient->setUser(pUser);
//    pRtspTcpClient->setStreamCallback(cbStreamData);
//    pRtspTcpClient->setConnectStatusListener(cbConnectStatus);
//    if (!pRtspTcpClient->startUp()) {
//        delete pRtspTcpClient;
//        return false;
//    }
//    rtspStreamHandle = (long) pRtspTcpClient;
//    return true;
//}
//
//VMNET_API void CALL_METHOD VmNet_StopStreamByRtsp(long rtspStreamHandle) {
//    LOGD("VMSDK", "VmNet_StopStreamByRtsp\n");
//    Dream::RtspTcpClient *pRtspTcpClient = (Dream::RtspTcpClient *) rtspStreamHandle;
//    if (pRtspTcpClient == nullptr) {
//        return;
//    }
//    pRtspTcpClient->shutdown();
//    delete pRtspTcpClient;
//}
//
//VMNET_API bool CALL_METHOD VmNet_PauseStreamByRtsp(long rtspStreamHandle) {
//    LOGD("VMSDK", "VmNet_PauseStreamByRtsp\n");
//    Dream::RtspTcpClient *pRtspTcpClient = (Dream::RtspTcpClient *) rtspStreamHandle;
//    if (pRtspTcpClient != nullptr) {
//        return pRtspTcpClient->pause();
//    }
//    return false;
//}
//
//VMNET_API bool CALL_METHOD VmNet_PlayStreamByRtsp(long rtspStreamHandle) {
//    LOGD("VMSDK", "VmNet_PlayStreamByRtsp\n");
//    Dream::RtspTcpClient *pRtspTcpClient = (Dream::RtspTcpClient *) rtspStreamHandle;
//    if (pRtspTcpClient != nullptr) {
//        return pRtspTcpClient->play();
//    }
//    return false;
//}

VMNET_API unsigned CALL_METHOD
VmNet_StartStreamByRtsp(const char *rtspUrl, fStreamCallBackV2 cbRealData,
                        fStreamConnectStatusCallBackV2 cbStatus, void *pUser, VMNET_OUT
                        unsigned &rtspStreamId, bool encrypt) {
    LOGI("VMSDK", "VmNet_StartStreamByRtsp\n");

    if (g_pMainModule == nullptr) {
        return ERR_CODE_SDK_UNINIT;
    }
    return g_pMainModule->startRtspStream(rtspUrl == nullptr ? "" : rtspUrl, cbRealData,
                                          cbStatus, pUser, rtspStreamId, encrypt);
}

VMNET_API void CALL_METHOD VmNet_StopStreamByRtsp(unsigned rtspStreamId) {
    LOGI("VMSDK", "VmNet_StopStreamByRtsp(%d)\n", rtspStreamId);
    if (g_pMainModule == nullptr) {
        return;
    }
    g_pMainModule->stopRtspStream(rtspStreamId);
}

VMNET_API unsigned CALL_METHOD VmNet_PauseStreamByRtsp(unsigned rtspStreamId) {
    LOGI("VMSDK", "VmNet_PauseStreamByRtsp(%d)\n", rtspStreamId);

    if (g_pMainModule == nullptr) {
        return ERR_CODE_SDK_UNINIT;
    }
    return g_pMainModule->pauseRtspStream(rtspStreamId);
}

VMNET_API unsigned CALL_METHOD VmNet_PlayStreamByRtsp(unsigned rtspStreamId) {
    LOGI("VMSDK", "VmNet_PlayStreamByRtsp(%d)\n", rtspStreamId);

    if (g_pMainModule == nullptr) {
        return ERR_CODE_SDK_UNINIT;
    }
    return g_pMainModule->playRtspStream(rtspStreamId);
}

VMNET_API unsigned CALL_METHOD VmNet_SpeedStreamByRtsp(unsigned rtspStreamId, float speed) {
    LOGI("VMSDK", "VmNet_SpeedStreamByRtsp(%d, %f)\n", rtspStreamId, speed);

    if (g_pMainModule == nullptr) {
        return ERR_CODE_SDK_UNINIT;
    }
    return g_pMainModule->speedRtspStream(rtspStreamId, speed);
}

VMNET_API bool CALL_METHOD VmNet_StreamIsValid(unsigned streamId) {
    if (g_pMainModule == nullptr) {
        return false;
    }
    return g_pMainModule->streamIsValid(streamId);
}

VMNET_API void CALL_METHOD VmNet_SendControl(const char *sFdId, int nChannelId,
                                             unsigned uControlType, unsigned uParm1,
                                             unsigned uParm2) {
    LOGD("VMSDK", "VmNet_SendControl\n");
    if (g_pMainModule == nullptr) {
        return;
    }
    g_pMainModule->sendCmd(sFdId, nChannelId, uControlType, uParm1, uParm2);
}

VMNET_API bool CALL_METHOD VmNet_FilterRtpHeader(const char *inData, int inLen,
                                                 VMNET_OUT char *payloadData, VMNET_OUT
                                                 int &payloadLen, VMNET_OUT
                                                 int &payloadType, VMNET_OUT int &seqNumber,
                                                 VMNET_OUT int &timestamp, VMNET_OUT bool &isMark) {
    Dream::RtpPacket *pRtpPacket = new Dream::RtpPacket();
    pRtpPacket->ForceFu();
    if (pRtpPacket->Parse((char *) inData, inLen) == 0) {
        if (payloadLen > pRtpPacket->m_nPayloadDataLen) {
            memcpy(payloadData, pRtpPacket->m_pPayloadData, (size_t) pRtpPacket->m_nPayloadDataLen);
            payloadLen = pRtpPacket->m_nPayloadDataLen;
            payloadType = pRtpPacket->m_nPayloadType;
            seqNumber = pRtpPacket->m_nSequenceNumber;
            timestamp = pRtpPacket->m_nTimeStamp;
            isMark = pRtpPacket->m_bMarker;
//            if (pRtpPacket->m_nPayloadType == 96) {
            delete pRtpPacket;
            return true;
//            }
//            LOGE("!!", "payloadtype[%d]\n", pRtpPacket->m_nPayloadType);
        }
    }
    delete pRtpPacket;
    return false;
}

VMNET_API bool CALL_METHOD VmNet_FilterRtpHeader_EXT(const char *inData, int inLen,
                                                     VMNET_OUT char *payloadData, VMNET_OUT
                                                     int &payloadLen, VMNET_OUT
                                                     int &payloadType, VMNET_OUT int &seqNumber,
                                                     VMNET_OUT int &timestamp, VMNET_OUT
                                                     bool &isMark, VMNET_OUT bool &isJWHeader,
                                                     VMNET_OUT bool &isFirstFrame, VMNET_OUT
                                                     bool &isLastFrame, VMNET_OUT
                                                     uint64_t &utcTimeStamp) {
    Dream::RtpPacket *pRtpPacket = new Dream::RtpPacket();
    pRtpPacket->ForceFu();
    if (pRtpPacket->Parse((char *) inData, inLen) == 0) {
        if (payloadLen > pRtpPacket->m_nPayloadDataLen) {
            memcpy(payloadData, pRtpPacket->m_pPayloadData, (size_t) pRtpPacket->m_nPayloadDataLen);
            payloadLen = pRtpPacket->m_nPayloadDataLen;
            payloadType = pRtpPacket->m_nPayloadType;
            seqNumber = pRtpPacket->m_nSequenceNumber;
            timestamp = pRtpPacket->m_nTimeStamp;
            isMark = pRtpPacket->m_bMarker;
            isJWHeader = pRtpPacket->m_bJWExtHeader;
            isFirstFrame = pRtpPacket->m_isFirstFrame == 1;
            isLastFrame = pRtpPacket->m_isLastFrame == 1;
            utcTimeStamp = pRtpPacket->m_utcTimeStamp;
            delete pRtpPacket;
            return true;
        }
    }
    delete pRtpPacket;
    return false;
}
