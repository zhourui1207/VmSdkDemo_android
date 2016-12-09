/*
 * StartTalkPacket.h
 *
 *  Created on: 2016年12月2日
 *      Author: zhourui
 *      开始对讲
 */

#ifndef STARTTALKPACKET_H_
#define STARTTALKPACKET_H_

#include "../../util/MsgPacket.h"
#include "PacketMsgType.h"

namespace Dream {

class StartTalkReqPacket: public MsgPacket {
public:
  enum ACTIVE_FLAG {
    ACTIVE_FLAG_NO_ACTIVE = 0,  // 无需激活
    ACTIVE_FLAG_RECEIVER_ACTIVE = 1,  // 接收端主动激活
    ACTIVE_FLAG_SENDER_ACTIVE = 2  // 发送端主动激活
  };

  enum TRANS_MODE {
    TRANS_MODE_UDP = 0,  // udp
    TRANS_MODE_TCP = 1  // tcp
  };

public:
  StartTalkReqPacket() :
      MsgPacket(MSG_START_TALK_REQ), _channelId(0), _talkSessionId(0), _activeFlag(
          ACTIVE_FLAG_NO_ACTIVE), _talkPort(0), _talkTransMode(
          TRANS_MODE_UDP) {

  }
  virtual ~StartTalkReqPacket() = default;

  // ---------------------派生类重写开始---------------------
  virtual std::size_t computeLength() const override {
    return MsgPacket::computeLength() + 5 * sizeof(int) + _fdId.length() + 1
        + _talkIp.length() + 1;
  }

  virtual int encode(char* pBuf, std::size_t len) override {
    std::size_t dataLength = computeLength();
    if (len < dataLength) {
      return -1;
    }
    int usedLen = MsgPacket::encode(pBuf, len);
    if (usedLen < 0) {
      return -1;
    }

    char* pBody = pBuf + usedLen;

    int pos = 0;
    ENCODE_STRING(pBody + pos, _fdId, pos);
    ENCODE_INT(pBody + pos, _channelId, pos);
    ENCODE_INT(pBody + pos, _talkSessionId, pos);
    ENCODE_INT(pBody + pos, _activeFlag, pos);
    ENCODE_STRING(pBody + pos, _talkIp, pos);
    ENCODE_INT(pBody + pos, _talkPort, pos);
    ENCODE_INT(pBody + pos, _talkTransMode, pos);
    return dataLength;
  }

  virtual int decode(char* pBuf, std::size_t len) override {
    int usedLen = MsgPacket::decode(pBuf, len);
    if (usedLen < 0) {
      return -1;
    }

    char* pBody = pBuf + usedLen;

    int pos = 0;
    DECODE_STRING(pBody + pos, _fdId, pos);
    DECODE_INT(pBody + pos, _channelId, pos);
    DECODE_INT(pBody + pos, _talkSessionId, pos);
    DECODE_INT(pBody + pos, _activeFlag, pos);
    DECODE_STRING(pBody + pos, _talkIp, pos);
    DECODE_INT(pBody + pos, _talkPort, pos);
    DECODE_INT(pBody + pos, _talkTransMode, pos);

    // 包长和实际长度不符
    if (computeLength() != length()) {
      return -1;
    }
    return length();
  }
  // ---------------------派生类重写结束---------------------

public:
  std::string _fdId;
  int _channelId;
  unsigned _talkSessionId;
  unsigned _activeFlag;
  std::string _talkIp;
  unsigned _talkPort;
  unsigned _talkTransMode;
};

class StartTalkRespPacket: public MsgPacket {
public:
  enum ACTIVE_FLAG {
    ACTIVE_FLAG_NO_ACTIVE = 0,  // 无需激活
    ACTIVE_FLAG_RECEIVER_ACTIVE = 1,  // 接收端主动激活
    ACTIVE_FLAG_SENDER_ACTIVE = 2  // 发送端主动激活
  };

  enum TRANS_MODE {
    TRANS_MODE_UDP = 0,  // udp
    TRANS_MODE_TCP = 1  // tcp
  };

public:
  StartTalkRespPacket() :
      MsgPacket(MSG_START_TALK_RESP), _channelId(0), _talkSessionId(0), _activeFlag(
          ACTIVE_FLAG_NO_ACTIVE), _talkPort(0), _talkTransMode(
          TRANS_MODE_UDP) {

  }
  virtual ~StartTalkRespPacket() = default;

  // ---------------------派生类重写开始---------------------
  virtual std::size_t computeLength() const override {
    return MsgPacket::computeLength() + 5 * sizeof(int) + _fdId.length() + 1
        + _talkIp.length() + 1;
  }

  virtual int encode(char* pBuf, std::size_t len) override {
    std::size_t dataLength = computeLength();
    if (len < dataLength) {
      return -1;
    }
    int usedLen = MsgPacket::encode(pBuf, len);
    if (usedLen < 0) {
      return -1;
    }

    char* pBody = pBuf + usedLen;

    int pos = 0;
    ENCODE_STRING(pBody + pos, _fdId, pos);
    ENCODE_INT(pBody + pos, _channelId, pos);
    ENCODE_INT(pBody + pos, _talkSessionId, pos);
    ENCODE_INT(pBody + pos, _activeFlag, pos);
    ENCODE_STRING(pBody + pos, _talkIp, pos);
    ENCODE_INT(pBody + pos, _talkPort, pos);
    ENCODE_INT(pBody + pos, _talkTransMode, pos);
    return dataLength;
  }

  virtual int decode(char* pBuf, std::size_t len) override {
    int usedLen = MsgPacket::decode(pBuf, len);
    if (usedLen < 0) {
      return -1;
    }

    char* pBody = pBuf + usedLen;

    int pos = 0;
    DECODE_STRING(pBody + pos, _fdId, pos);
    DECODE_INT(pBody + pos, _channelId, pos);
    DECODE_INT(pBody + pos, _talkSessionId, pos);
    DECODE_INT(pBody + pos, _activeFlag, pos);
    DECODE_STRING(pBody + pos, _talkIp, pos);
    DECODE_INT(pBody + pos, _talkPort, pos);
    DECODE_INT(pBody + pos, _talkTransMode, pos);

    // 包长和实际长度不符
    if (computeLength() != length()) {
      return -1;
    }
    return length();
  }
  // ---------------------派生类重写结束---------------------

public:
  std::string _fdId;
  int _channelId;
  unsigned _talkSessionId;
  unsigned _activeFlag;
  std::string _talkIp;
  unsigned _talkPort;
  unsigned _talkTransMode;
};

} /* namespace Dream */

#endif /* STARTTALKPACKET_H_ */
