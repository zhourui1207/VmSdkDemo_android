/*
 * ActivePacket.h
 *
 *  Created on: 2016年9月28日
 *      Author: zhourui
 *      保活包
 *
 */

#ifndef ACTIVEPACKET_H_
#define ACTIVEPACKET_H_

#include "../../util/StreamPacket.h"
#include "PacketStreamType.h"

namespace Dream {

class ActivePacket: public StreamPacket {

public:
  ActivePacket() :
    StreamPacket(MSG_STREAM_ACTIVE) {

  }
  virtual ~ActivePacket() = default;

  // ---------------------派生类重写开始---------------------
  // 无包体，无需覆盖父类方法
  // ---------------------派生类重写结束---------------------
};

} /* namespace Dream */

#endif /* LOGINREQPACKET_H_ */
