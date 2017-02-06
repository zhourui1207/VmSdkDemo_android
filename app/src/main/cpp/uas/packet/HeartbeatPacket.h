/*
 * HeartbeatPacket.h
 *
 *  Created on: 2016年9月28日
 *      Author: zhourui
 *      心跳包
 *
 */

#ifndef HEARTBEATPACKET_H_
#define HEARTBEATPACKET_H_

#include "../../util/MsgPacket.h"
#include "PacketMsgType.h"

namespace Dream {

// 心跳请求包
    class HeartbeatReqPacket : public MsgPacket {

    public:
        HeartbeatReqPacket() :
                MsgPacket(MSG_HEARTBEAT_REQ) {

        }

        virtual ~HeartbeatReqPacket() = default;

        // ---------------------派生类重写开始---------------------
        // 无包体，无需覆盖父类方法
        // ---------------------派生类重写结束---------------------
    };

// 心跳响应包
    class HeartbeatRespPacket : public MsgPacket {
    public:
        HeartbeatRespPacket() :
                MsgPacket(MSG_HEARTBEAT_RESP) {

        }

        virtual ~HeartbeatRespPacket() = default;

        // ---------------------派生类重写开始---------------------
        // 无包体，无需覆盖父类方法
        // ---------------------派生类重写结束---------------------
    };

} /* namespace Dream */

#endif /* LOGINREQPACKET_H_ */
