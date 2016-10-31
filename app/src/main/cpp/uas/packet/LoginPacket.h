/*
 * LoginPacket.h
 *
 *  Created on: 2016年9月28日
 *      Author: zhourui
 *      登录包
 *
 */

#ifndef LOGINPACKET_H_
#define LOGINPACKET_H_

#include "../../util/MsgPacket.h"
#include "PacketMsgType.h"

namespace Dream {

// 登录请求包
class LoginReqPacket: public MsgPacket {
public:
  enum LOGIN_MODE {
    LOGIN_MODE_ACCOUNT_PWD = 0,  // 账号密码登录
    LOGIN_MODE_ACCOUNT_PWD_USDKEY = 1  // 账号密码＋usbKey登录
  };

  enum LOGIN_TYPE {
    LOGIN_TYPE_PC = 0,  // 电脑客户端
    LOGIN_TYPE_OCX = 1,  // ocx插件
    LOGIN_TYPE_MOBILE = 2  // 手机
  };

  enum {
    NONE_NAT = 0,  // 不使用nat穿越
    IS_NAT = 1  // 使用nat穿越
  };

public:
  LoginReqPacket() :
      MsgPacket(MSG_LOGIN_REQ), _loginMode(0), _loginType(0), _isNat(0) {

  }
  virtual ~LoginReqPacket() = default;

  // ---------------------派生类重写开始---------------------
  virtual std::size_t computeLength() const override {
    return MsgPacket::computeLength() + 3 * sizeof(int)
        + _loginName.length() + 1 + _loginPwd.length() + 1 + _loginSms.length()
        + 1 + _srcIp.length() + 1;
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
    ENCODE_STRING(pBody + pos, _loginName, pos);
    ENCODE_STRING(pBody + pos, _loginPwd, pos);
    ENCODE_STRING(pBody + pos, _loginSms, pos);
    ENCODE_INT(pBody + pos, _loginMode, pos);
    ENCODE_INT(pBody + pos, _loginType, pos);

    ENCODE_STRING(pBody + pos, _srcIp, pos);
    ENCODE_INT(pBody + pos, _isNat, pos);

    return dataLength;
  }

  virtual int decode(char* pBuf, std::size_t len) override {
    int usedLen = MsgPacket::decode(pBuf, len);
    if (usedLen < 0) {
      return -1;
    }

    char* pBody = pBuf + usedLen;

    int pos = 0;
    DECODE_STRING(pBody + pos, _loginName, pos);
    DECODE_STRING(pBody + pos, _loginPwd, pos);
    DECODE_STRING(pBody + pos, _loginSms, pos);
    DECODE_INT(pBody + pos, _loginMode, pos);
    DECODE_INT(pBody + pos, _loginType, pos);

    DECODE_STRING(pBody + pos, _srcIp, pos);
    DECODE_INT(pBody + pos, _isNat, pos);

    // 包长和实际长度不符
    if (computeLength() != length()) {
      return -1;
    }
    return length();
  }
  // ---------------------派生类重写结束---------------------

public:
  std::string _loginName;
  std::string _loginPwd;
  std::string _loginSms;
  unsigned _loginMode;
  unsigned _loginType;

  std::string _srcIp;
  unsigned _isNat;
};

// 登录响应包
class LoginRespPacket: public MsgPacket {
public:
  LoginRespPacket() :
      MsgPacket(MSG_LOGIN_RESP) {

  }
  virtual ~LoginRespPacket() = default;

  // ---------------------派生类重写开始---------------------
  // 无包体，无需覆盖父类方法
  // ---------------------派生类重写结束---------------------
};

} /* namespace Dream */

#endif /* LOGINREQPACKET_H_ */
