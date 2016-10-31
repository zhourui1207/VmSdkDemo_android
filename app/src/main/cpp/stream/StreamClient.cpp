/*
 * StreamClient.cpp
 *
 *  Created on: 2016年10月13日
 *      Author: zhourui
 */

#include "packet/ActivePacket.h"
#include "packet/AudioPacket.h"
#include "packet/VideoPacket.h"
#include "StreamClient.h"

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
  default:
    printf("未匹配到相应的包! msgType=[%d]\n", msgType);
    break;
  }
  if (isMatch) {
    packetPtr.reset(new StreamPacket(msgType));
  }
  return packetPtr;
}

void StreamClient::receive(const std::shared_ptr<StreamPacket>& packetPtr) {
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
  default: {
    printf("非码流包!\n");
    break;
  }
  }
  if (isStream && _streamCallback) {
    auto streamDataPtr = std::make_shared<StreamData>(streamType, packetPtr->dataLen(), packetPtr->data());
    _streamCallback(streamDataPtr);
  }
}

void StreamClient::onConnect() {
  printf("已连接，开启心跳定时器\n");
  if (_timerPtr.get() != nullptr) {
    _timerPtr->cancel();
  }
  _timerPtr.reset(new Timer(std::bind(&StreamClient::heartbeat, this), 0, true, _heartbeatInterval));
  _timerPtr->start();
}

void StreamClient::onClose() {
  if (_timerPtr.get() != nullptr) {
    printf("已断开，取消心跳定时器\n");
    _timerPtr->cancel();
    _timerPtr.reset();
  }
}

void StreamClient::heartbeat() {
  ActivePacket req;
  send(req);
}

} /* namespace Dream */
