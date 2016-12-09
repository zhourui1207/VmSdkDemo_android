/*
 * VmNet.cpp
 *
 *  Created on: 2016年10月14日
 *      Author: zhourui
 *      在这一层加入了gbk转utf－8功能，默认开启，若需要禁止转换，#define GBK_2_UTF8_DISABLE
 */

#include "../util/public/StringUtil.h"
#include "ErrorCode.h"
#include "MainModule.h"
#include "VmNet.h"

#define GBK2UTF8_TMP_MAX_LEN 1024  // gbk转tmp时使用的临时变量最大长度

Dream::MainModule* g_pMainModule = nullptr;


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

int GBK2UTF8_STR(const std::string& inStr, char *outbuf, size_t outlen) {
  return GBK2UTF8(inStr.c_str(), inStr.length(), outbuf, outlen);
}

VMNET_API bool CALL_METHOD VmNet_Init(unsigned uMaxThreadCount) {
  if (g_pMainModule == nullptr) {
    g_pMainModule = new Dream::MainModule(uMaxThreadCount);
    return true;
  }
  return false;
}

VMNET_API void CALL_METHOD VmNet_UnInit() {
  if (g_pMainModule != nullptr) {
    delete g_pMainModule;
    g_pMainModule = nullptr;
  }
}

VMNET_API unsigned CALL_METHOD VmNet_Connect(const char* sServerAddr,
                                             unsigned uServerPort, fUasConnectStatusCallBack cbUasConnectStatus) {
  if (g_pMainModule == nullptr) {
    return ERR_CODE_SDK_UNINIT;
  }
  g_pMainModule->setUasConnectStatusCallback(cbUasConnectStatus);
  return g_pMainModule->connect(sServerAddr, uServerPort);
}

// 断开服务器
VMNET_API void CALL_METHOD VmNet_Disconnect() {
  if (g_pMainModule == nullptr) {
    return;
  }
  g_pMainModule->cancelUasConnectStatusCallback();
  g_pMainModule->disconnect();
}

VMNET_API unsigned CALL_METHOD VmNet_Login(const char* sLoginName,
                                           const char* sLoginPwd) {
  if (g_pMainModule == nullptr) {
    return ERR_CODE_SDK_UNINIT;
  }
  return g_pMainModule->login(sLoginName, sLoginPwd);
}

VMNET_API void CALL_METHOD VmNet_Logout() {
  if (g_pMainModule == nullptr) {
    return;
  }
  g_pMainModule->logout();
}

VMNET_API unsigned CALL_METHOD VmNet_GetDepTrees(unsigned uPageNo,
                                                 unsigned uPageSize, out TVmDepTree* pDepTrees, out unsigned& uSize) {
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
                                                 unsigned uPageSize, int nDepId, out TVmChannel* pChannels,
                                                 out unsigned& uSize) {
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
                                                unsigned uPageSize, const char* sFdId, int nChannelId, unsigned uBeginTime,
                                                unsigned uEndTime, bool bIsCenter, out TVmRecord* pRecords,
                                                out unsigned& uSize) {
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
                                               unsigned uPageSize, const char* sFdId, int nChannelId,
                                               unsigned uChannelBigType, const char* sBeginTime, const char* sEndTime,
                                               out TVmAlarm* pAlarms, out unsigned& uSize) {
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
  if (g_pMainModule == nullptr) {
    return;
  }
  g_pMainModule->startReceiveRealAlarm(cbRealAlarm);
}

VMNET_API void CALL_METHOD VmNet_StopReceiveRealAlarm() {
  if (g_pMainModule == nullptr) {
    return;
  }
  g_pMainModule->stopReceiveRealAlarm();
}

VMNET_API unsigned CALL_METHOD VmNet_OpenRealplayStream(const char* sFdId,
                                                        int nChannelId, bool bIsSub, out unsigned& uMonitorId, out char* sVideoAddr,
                                                        out unsigned& uVideoPort, out char* sAudioAddr, out unsigned& uAudioPort) {
  if (g_pMainModule == nullptr) {
    return ERR_CODE_SDK_UNINIT;
  }
  std::string videoAddr, audioAddr;
  unsigned ret = g_pMainModule->openRealplayStream(sFdId, nChannelId, bIsSub,
                                                   uMonitorId, videoAddr, uVideoPort, audioAddr, uAudioPort);
  if (ret == ERR_CODE_OK) {
    strcpy(sVideoAddr, videoAddr.c_str());
    strcpy(sAudioAddr, audioAddr.c_str());
  }
  return ret;
}

VMNET_API void CALL_METHOD VmNet_CloseRealplayStream(unsigned uMonitorId) {
  if (g_pMainModule == nullptr) {
    return;
  }
  g_pMainModule->closeRealplayStream(uMonitorId);
}

VMNET_API unsigned CALL_METHOD VmNet_OpenPlaybackStream(const char* sFdId,
                                                        int nChannelId, bool bIsCenter, unsigned beginTime, unsigned endTime,
                                                        out unsigned& uMonitorId, out char* sVideoAddr, out unsigned& uVideoPort,
                                                        out char* sAudioAddr, out unsigned& uAudioPort) {
  if (g_pMainModule == nullptr) {
    return ERR_CODE_SDK_UNINIT;
  }
  std::string videoAddr, audioAddr;
  unsigned ret = g_pMainModule->openPlaybackStream(sFdId, nChannelId, bIsCenter,
                                                   beginTime, endTime, uMonitorId, videoAddr, uVideoPort, audioAddr,
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
  if (g_pMainModule == nullptr) {
    return;
  }
  g_pMainModule->closePlaybackStream(uMonitorId);
}

// 控制回放
// 入参    uMonitorId：监控id，打开回放码流时获得； bIsCenter：是否为中心录像
//        uControlId：控制id； sAction：操作； sParam：参数
VMNET_API unsigned CALL_METHOD VmNet_ControlPlayback(unsigned uMonitorId, unsigned uControlId, const char* sAction, const char* sParam) {
  if (g_pMainModule == nullptr) {
    return ERR_CODE_SDK_UNINIT;
  }
  return g_pMainModule->controlPlayback(uMonitorId, uControlId, sAction, sParam);
}

VMNET_API unsigned CALL_METHOD VmNet_StartStream(const char* sAddr,
                                                 unsigned uPort, fStreamCallBack cbRealData,
                                                 fStreamConnectStatusCallBack cbConnectStatus, void* pUser,
                                                 out unsigned& uStreamId) {
  if (g_pMainModule == nullptr) {
    return ERR_CODE_SDK_UNINIT;
  }
  return g_pMainModule->startStream(sAddr, uPort, cbRealData, cbConnectStatus,
                                    pUser, uStreamId);
}

VMNET_API void CALL_METHOD VmNet_StopStream(unsigned uStreamId) {
  if (g_pMainModule == nullptr) {
    return;
  }
  g_pMainModule->stopStream(uStreamId);
}

VMNET_API void CALL_METHOD VmNet_SendControl(const char* sFdId, int nChannelId,
                                             unsigned uControlType, unsigned uParm1, unsigned uParm2) {
  if (g_pMainModule == nullptr) {
    return;
  }
  g_pMainModule->sendCmd(sFdId, nChannelId, uControlType, uParm1, uParm2);
}
