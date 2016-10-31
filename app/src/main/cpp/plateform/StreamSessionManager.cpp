/*
 * StreamSessionManager.cpp
 *
 *  Created on: 2016年10月19日
 *      Author: zhourui
 */

#include "../rtp/RtpPacket.h"
#include "StreamSessionManager.h"

#ifdef _ANDROID
#include <jni.h>
#include <Android/log.h>

extern JavaVM* g_pJavaVM;  // 定义外部变量，该变量在VmNet-lib.cpp中被定义和赋值
#endif

namespace Dream {

StreamSession::StreamSession(unsigned streamId, const std::string& addr,
    unsigned port, fStreamCallBack streamCallback,
    fStreamConnectStatusCallBack streamConnectStatusCallback, void* user) :
    _streamId(streamId), _streamClient(addr, port,
        std::bind(&StreamSession::onStream, this, std::placeholders::_1)), _streamCallback(
        streamCallback), _streamConnectStatusCallback(
        streamConnectStatusCallback), _user(user) {
  _streamClient.setConnectStatusListener(
      std::bind(&StreamSession::onConnectStatus, this, std::placeholders::_1));
}

StreamSession::~StreamSession() {
#ifdef _ANDROID
  if (g_pJavaVM && _user) {
    JNIEnv* jniEnv = nullptr;
    if (g_pJavaVM->GetEnv(reinterpret_cast<void**>(&jniEnv), JNI_VERSION_1_6) == JNI_OK) {
      if (jniEnv) {
        jniEnv->DeleteGlobalRef(static_cast<jobject>(_user));
        __android_log_print(ANDROID_LOG_WARN, "StreamSession", "删出android线程[%zd]的全局引用！[%zd]", jniEnv, _user);
      }
    }
  }
#endif
  _streamClient.cancelStreamListener();
//  _streamClient.cancelConnectStatusListener(); // 将这段注释掉，可以获取到stream断开的消息
}

bool StreamSession::startUp() {
  return _streamClient.startUp();
}

void StreamSession::onStream(const std::shared_ptr<StreamData>& streamDataPtr) {
  if (_streamCallback) {
    // 去掉rtp头后再回调
    auto rtpPacketPtr = std::unique_ptr<RtpPacket>(new RtpPacket);
    if (rtpPacketPtr->Parse(streamDataPtr->data(), streamDataPtr->length()) == 0) {
      _streamCallback(_streamId, streamDataPtr->streamType(),
          rtpPacketPtr->GetPayloadType(), rtpPacketPtr->GetPayloadData(),
          rtpPacketPtr->GetPayloadLength(), rtpPacketPtr->GetTimestamp(),
          rtpPacketPtr->GetSequenceNumber(), rtpPacketPtr->HasMarker(), _user);
    } else {
      printf("rtp 解析失败\n");
    }
  }
}

void StreamSession::onConnectStatus(bool isConnected) {
  if (_streamConnectStatusCallback) {
    _streamConnectStatusCallback(_streamId, isConnected, _user);
  }
}

StreamSessionManager::StreamSessionManager() :
    _streamId(0) {
}

StreamSessionManager::~StreamSessionManager() {
  clearAll();
}

bool StreamSessionManager::addStreamClient(const std::string& addr,
    unsigned port, fStreamCallBack streamCallback,
    fStreamConnectStatusCallBack streamConnectStatusCallback, void* pUser,
    unsigned& streamId) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (!getStreamId(streamId)) {  // 连接数到上限
    return false;
  }

  auto streamSessionPtr = std::make_shared<StreamSession>(streamId, addr, port,
      streamCallback, streamConnectStatusCallback, pUser);
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
    _sessionMap.erase(streamSessionPtrIt);
    recoveryStreamId(streamId);
  }
}

// 清除所有码流连接
void StreamSessionManager::clearAll() {
  std::lock_guard<std::mutex> lock(_mutex);
  _sessionMap.clear();
  _streamId = 0;
  _recoveryStreamIds.clear();
}

// 获得streamid 从 1 - uint_max
bool StreamSessionManager::getStreamId(unsigned& streamId) {
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
