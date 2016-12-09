/*
 * UasClient.cpp
 *
 *  Created on: 2016年9月30日
 *      Author: zhourui
 */

#include "packet/ControlPacket.h"
#include "packet/ControlPlaybackPacket.h"
#include "packet/HeartbeatPacket.h"
#include "packet/LoginPacket.h"
#include "packet/LogoutPacket.h"
#include "packet/PacketMsgType.h"
#include "packet/PushAlarmPacket.h"
#include "packet/StartMonitorPacket.h"
#include "packet/StartPlaybackPacket.h"
#include "packet/StopMonitorPacket.h"
#include "packet/StopPlaybackPacket.h"
#include "UasClient.h"

namespace Dream {

std::shared_ptr<MsgPacket> UasClient::newPacketPtr(unsigned msgType) {
  std::shared_ptr<MsgPacket> packetPtr;
  switch (msgType) {
  case MSG_LOGIN_RESP:
    printf("登录返回包\n");
    packetPtr.reset(new LoginRespPacket());
    break;
  case MSG_LOGOUT_RESP:
    printf("登出返回包\n");
    break;
  case MSG_HEARTBEAT_RESP:
    printf("心跳返回包\n");
    break;
  case MSG_GET_DEP_TREE_RESP:
    printf("获取行政树返回包\n");
    packetPtr.reset(new GetDepTreeRespPacket());
    break;
  case MSG_GET_DEP_CHANNEL_RESP:
    printf("获取通道返回包\n");
    packetPtr.reset(new GetChannelRespPacket());
    break;
  case MSG_GET_RECORD_RESP:
    printf("获取录像返回包\n");
    packetPtr.reset(new GetRecordRespPacket());
    break;
  case MSG_GET_ALARM_RESP:
    printf("获取报警返回包\n");
    packetPtr.reset(new GetAlarmRespPacket());
    break;
  case MSG_START_MONITOR_RESP:
    printf("开始预览返回包\n");
    packetPtr.reset(new StartMonitorRespPacket());
    break;
  case MSG_STOP_MONITOR_RESP:
    printf("停止预览返回包\n");
    break;
  case MSG_START_PLAYBACK_RESP:
    printf("开始回放返回包\n");
    packetPtr.reset(new StartPlaybackRespPacket());
    break;
  case MSG_STOP_PLAYBACK_RESP:
    printf("停止回放返回包\n");
    break;
  case MSG_CONTROL_PLAYBACK_RESP:
    printf("控制回放返回包\n");
    break;
  case MSG_CONTROL_RESP:
    printf("控制返回包\n");
    break;
  case MSG_ALARM_NOTIFY:
    printf("报警推送消息\n");
    packetPtr.reset(new PushAlarmPacket());
    break;
  default:
    printf("未匹配到相应的包! msgType=[%d]\n", msgType);
    break;
  }
  return packetPtr;
}

void UasClient::receive(const std::shared_ptr<MsgPacket>& packetPtr) {
  bool isResp = false;
  unsigned msgType = packetPtr->msgType();
  switch (msgType) {
  case MSG_LOGIN_RESP: {
    isResp = true;
    break;
  }
  case MSG_GET_DEP_TREE_RESP: {
    isResp = true;
    break;
  }
  case MSG_GET_DEP_CHANNEL_RESP: {
    isResp = true;
    break;
  }
  case MSG_GET_RECORD_RESP: {
    isResp = true;
    break;
  }
  case MSG_GET_ALARM_RESP: {
    isResp = true;
    break;
  }
  case MSG_START_MONITOR_RESP: {
    isResp = true;
    break;
  }
  case MSG_START_PLAYBACK_RESP: {
    isResp = true;
    break;
  }
  case MSG_CONTROL_PLAYBACK_RESP: {
    isResp = true;
    break;
  }
  case MSG_ALARM_NOTIFY: {
    // 处理报警通知
    handleAlarmNotity(packetPtr);
    break;
  }
  default: {
    printf("未处理包!\n");
    break;
  }
  }
  if (isResp) {
    handleResp(packetPtr);
  }
}

void UasClient::handleResp(const std::shared_ptr<MsgPacket>& packetPtr) {
  std::lock_guard<std::mutex> lock(_mutex);
  unsigned seqNumber = packetPtr->seqNumber();
  auto packetPtrIt = _receivePackets.find(seqNumber);
  if (packetPtrIt != _receivePackets.end()) {
    packetPtrIt->second = packetPtr;
    _receiveSeqNumber.store(packetPtr->seqNumber());
    _condition.notify_all();
  }
}

void UasClient::handleAlarmNotity(const std::shared_ptr<MsgPacket>& packetPtr) {
  PushAlarmPacket* pPacket = static_cast<PushAlarmPacket*>(packetPtr.get());
  if (_alarmListener) {
    _alarmListener(pPacket->_fdId, pPacket->_channelId, pPacket->_alarmType,
        pPacket->_parameter1, pPacket->_parameter2);
  }
}

bool UasClient::sendAndWaitRespPacket(MsgPacket& msgPacket,
    std::shared_ptr<MsgPacket>& respPacket) {
  // 需要阻塞等待响应包，或者超时返回
  std::unique_lock<std::mutex> lock(_mutex);
  // 写入序列号
  unsigned seqNumber = generateSeqNumber();
  msgPacket.seqNumber(seqNumber);
  // 准备接收响应包
  _receivePackets.emplace(
      std::make_pair(seqNumber, std::shared_ptr<MsgPacket>(nullptr)));

  bool sendSuccess = send(msgPacket);
  // 发送包
  if (sendSuccess) {  // 如果发送成功，等待返回；若失败的话，则立刻返回
    // 线程挂起
    _condition.wait_for(lock, std::chrono::milliseconds(_timeout),
        [&]()->bool {return _receiveSeqNumber.load() == seqNumber;});
  }

  // 等到响应包或者超时或者发送失败
  auto packetPtrIt = _receivePackets.find(seqNumber);
  if (packetPtrIt != _receivePackets.end()) {
    respPacket = packetPtrIt->second;
  }

  // 删除等待响应包内存
  _receivePackets.erase(packetPtrIt);

  return sendSuccess;
}

void UasClient::onConnect() {
  printf("已连接，开启心跳定时器\n");
  if (_timerPtr.get() != nullptr) {
    _timerPtr->cancel();
  }
  _timerPtr.reset(
      new Timer(std::bind(&UasClient::heartbeat, this), 0, true,
          _heartbeatInterval));
  _timerPtr->start();
}

void UasClient::onClose() {
  if (_timerPtr.get() != nullptr) {
    printf("已断开，关闭心跳定时器\n");
    _timerPtr->cancel();
    _timerPtr.reset();
  }
}

unsigned UasClient::login(const std::string& loginName,
    const std::string& loginPwd) {
  LoginReqPacket req;
  req._loginName = loginName;
  req._loginPwd = loginPwd;
  req._loginMode = LoginReqPacket::LOGIN_MODE_ACCOUNT_PWD;
  req._loginType = LoginReqPacket::LOGIN_TYPE_MOBILE;
  req._isNat = false;

  std::shared_ptr<MsgPacket> respPtr(nullptr);
  if (!sendAndWaitRespPacket(req, respPtr)) {
    return STATUS_SEND_FAILED;
  }
  if (respPtr.get() == nullptr) {
    return STATUS_TIMEOUT_ERR;
  }
  return respPtr->status();
}

void UasClient::logout() {
  LogoutReqPacket req;
  send(req);  // 不需要等待响应
}

unsigned UasClient::getDepTrees(DepTreeList& depTrees) {
  GetDepTreeReqPacket req;

  std::shared_ptr<MsgPacket> respPtr(nullptr);
  if (!sendAndWaitRespPacket(req, respPtr)) {
    return STATUS_SEND_FAILED;
  }
  if (respPtr.get() == nullptr) {
    return STATUS_TIMEOUT_ERR;
  }

  GetDepTreeRespPacket* pResp =
      static_cast<GetDepTreeRespPacket*>(respPtr.get());
  int ret = pResp->status();
  if (ret == STATUS_OK) {
    depTrees = pResp->_depTrees;
  }
  return ret;
}

unsigned UasClient::getChannels(int depId, ChannelList& channels) {
  GetChannelReqPacket req;
  req._depId = depId;

  std::shared_ptr<MsgPacket> respPtr(nullptr);
  if (!sendAndWaitRespPacket(req, respPtr)) {
    return STATUS_SEND_FAILED;
  }
  if (respPtr.get() == nullptr) {
    return STATUS_TIMEOUT_ERR;
  }

  GetChannelRespPacket* pResp =
      static_cast<GetChannelRespPacket*>(respPtr.get());
  int ret = pResp->status();
  if (ret == STATUS_OK) {
    channels = pResp->_channels;
  }
  return ret;
}

unsigned UasClient::getRecords(const std::string& fdId, int channelId,
    unsigned beginTime, unsigned endTime, bool isCenter, RecordList& records) {
  GetRecordReqPacket req;
  req._fdId = fdId;
  req._channelId = (channelId - 1 < 0) ? 0 : channelId - 1;
  req._beginTime = beginTime;
  req._endTime = endTime;
  req._recordType =
      isCenter ?
          GetRecordReqPacket::RECORD_TYPE_CENTER :
          GetRecordReqPacket::RECORD_TYPE_DEVICE;

  std::shared_ptr<MsgPacket> respPtr(nullptr);
  if (!sendAndWaitRespPacket(req, respPtr)) {
    return STATUS_SEND_FAILED;
  }
  if (respPtr.get() == nullptr) {
    return STATUS_TIMEOUT_ERR;
  }

  GetRecordRespPacket* pResp = static_cast<GetRecordRespPacket*>(respPtr.get());
  int ret = pResp->status();
  if (ret == STATUS_OK) {
    records = pResp->_records;
  }
  return ret;
}

unsigned UasClient::getAlarms(const std::string& fdId, int channelId,
    unsigned channelBigType, const std::string& beginTime,
    const std::string& endTime, AlarmList& alarms) {
  GetAlarmReqPacket req;
  req._fdId = fdId;
  req._channelId = channelId;
  req._channelBigType = channelBigType;
  req._beginTime = beginTime;
  req._endTime = endTime;

  std::shared_ptr<MsgPacket> respPtr(nullptr);
  if (!sendAndWaitRespPacket(req, respPtr)) {
    return STATUS_SEND_FAILED;
  }
  if (respPtr.get() == nullptr) {
    return STATUS_TIMEOUT_ERR;
  }

  GetAlarmRespPacket* pResp = static_cast<GetAlarmRespPacket*>(respPtr.get());
  int ret = pResp->status();
  if (ret == STATUS_OK) {
    alarms = pResp->_alarms;
  }
  return ret;
}

unsigned UasClient::openRealplayStream(const std::string& fdId, int channelId,
bool isSub, unsigned& monitorId, std::string& videoAddr, unsigned& videoPort,
    std::string& audioAddr, unsigned& audioPort) {
  StartMonitorReqPacket req;
  req._fdId = fdId;
  req._channelId = (channelId - 1 < 0) ? 0 : channelId - 1;
  req._subChannelId =
      isSub ?
          StartMonitorReqPacket::SUB_STREAM :
          StartMonitorReqPacket::MAIN_STREAM;
  monitorId = generateMonitorSessionId();
  req._monitorSessionId = monitorId;
  req._videoPort = 12001;
  req._videoTransMode = StartMonitorReqPacket::TRANS_MODE_TCP;
  req._audioPort = 12002;  // 音频端口必须大于0才会返回音频端口，使用tcp时，这个值没什么意义，只要大于0即可
  req._audioTransMode = StartMonitorReqPacket::TRANS_MODE_TCP;

  std::shared_ptr<MsgPacket> respPtr(nullptr);
  if (!sendAndWaitRespPacket(req, respPtr)) {
    return STATUS_SEND_FAILED;
  }
  if (respPtr.get() == nullptr) {
    return STATUS_TIMEOUT_ERR;
  }

  StartMonitorRespPacket* pResp =
      static_cast<StartMonitorRespPacket*>(respPtr.get());
  int ret = pResp->status();
  if (ret == STATUS_OK) {
    videoAddr = pResp->_videoIp;
    videoPort = pResp->_videoPort;
    audioAddr = pResp->_audioIp;
    audioPort = pResp->_audioPort;
  }
  return ret;
}

void UasClient::closeRealplayStream(unsigned monitorId) {
  StopMonitorReqPacket req;
  req._monitorSessionId = monitorId;

  send(req);
}

unsigned UasClient::openPlaybackStream(const std::string& fdId, int channelId,
bool isCenter, unsigned beginTime, unsigned endTime, unsigned& monitorId,
    std::string& videoAddr, unsigned& videoPort, std::string& audioAddr,
    unsigned& audioPort) {
  StartPlaybackReqPacket req;

  req._fdId = fdId;
  req._channelId = (channelId - 1 < 0) ? 0 : channelId - 1;
  req._subChannelId = StartPlaybackReqPacket::MAIN_STREAM;
  monitorId = generateMonitorSessionId();
  req._monitorSessionId = monitorId;
  req._recordType =
      isCenter ?
          StartPlaybackReqPacket::RECORD_TYPE_CENTER :
          StartPlaybackReqPacket::RECORD_TYPE_DEVICE;
  req._flag = StartPlaybackReqPacket::FLAG_PLAYBACK;
  req._startTime = beginTime;
  req._endTime = endTime;
  req._videoPort = 12001;
  req._videoTransMode = StartPlaybackReqPacket::TRANS_MODE_TCP;
  req._audioPort = 12002;  // 音频端口必须大于0才会返回音频端口，使用tcp时，这个值没什么意义，只要大于0即可
  req._audioTransMode = StartPlaybackReqPacket::TRANS_MODE_TCP;

  std::shared_ptr<MsgPacket> respPtr(nullptr);
  if (!sendAndWaitRespPacket(req, respPtr)) {
    return STATUS_SEND_FAILED;
  }
  if (respPtr.get() == nullptr) {
    return STATUS_TIMEOUT_ERR;
  }

  StartPlaybackRespPacket* pResp =
      static_cast<StartPlaybackRespPacket*>(respPtr.get());
  int ret = pResp->status();
  if (ret == STATUS_OK) {
    videoAddr = pResp->_videoIp;
    videoPort = pResp->_videoPort;
    audioAddr = pResp->_audioIp;
    audioPort = pResp->_audioPort;
    
    // 记录回放session
    _playbackSessionManager.AddSession(req._monitorSessionId, isCenter, req._fdId, req._channelId);
  }
  return ret;
}

void UasClient::closePlaybackStream(unsigned monitorId) {
  // 查找session是否存在
  auto sessionPtr = _playbackSessionManager.getSession(monitorId);
  if (sessionPtr.get() == nullptr) {
    return;
  }
  
  bool isCenter = sessionPtr->IsCenter();
  std::string fdId = sessionPtr->FdId();
  int channelId = sessionPtr->ChannelId();
  
  _playbackSessionManager.removeSession(monitorId);
  
  StopPlaybackReqPacket req;
  req._monitorSessionId = monitorId;
  req._fdId = fdId;
  req._channelId = channelId;
  req._recordType =
        isCenter ?
            StartPlaybackReqPacket::RECORD_TYPE_CENTER :
            StartPlaybackReqPacket::RECORD_TYPE_DEVICE;

  send(req);
}

unsigned UasClient::controlPlayback(unsigned monitorId, unsigned controlId,
    const std::string& action, const std::string& param) {
  // 查找session是否存在
  auto sessionPtr = _playbackSessionManager.getSession(monitorId);
  if (sessionPtr.get() == nullptr) {
    return STATUS_NOT_EXIST_SESSION;
  }
  
  bool isCenter = sessionPtr->IsCenter();
  std::string fdId = sessionPtr->FdId();
  int channelId = sessionPtr->ChannelId();
  
  ControlPlaybackReqPacket req;
  req._monitorSessionId = monitorId;
  req._fdId = fdId;
  req._channelId = channelId;
  req._recordType =
       isCenter ?
           StartPlaybackReqPacket::RECORD_TYPE_CENTER :
           StartPlaybackReqPacket::RECORD_TYPE_DEVICE;
  req._controlId = controlId;
  req._action = action;
  req._param = param;

  std::shared_ptr<MsgPacket> respPtr(nullptr);
  if (!sendAndWaitRespPacket(req, respPtr)) {
    return STATUS_SEND_FAILED;
  }
  if (respPtr.get() == nullptr) {
    return STATUS_TIMEOUT_ERR;
  }

  ControlPlaybackRespPacket* pResp =
      static_cast<ControlPlaybackRespPacket*>(respPtr.get());
  int ret = pResp->status();
  return ret;
}

void UasClient::sendCmd(const std::string& fdId, int channelId,
    unsigned controlType, unsigned param1, unsigned param2) {
  ControlReqPacket req;
  req._fdId = fdId;
  req._channelId = (channelId - 1 < 0) ? 0 : channelId - 1;
  req._controlType = controlType;
  req._parameter1 = param1;
  req._parameter2 = param2;

  send(req);
}

void UasClient::heartbeat() {
  HeartbeatReqPacket req;
  send(req);  // 不需要等待响应
}

} /* namespace Dream */
