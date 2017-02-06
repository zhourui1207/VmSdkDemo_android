/*
 * LogoutPacket.h
 *
 *  Created on: 2016年9月28日
 *      Author: zhourui
 *      登录包
 *
 */

#ifndef LOGOUTPACKET_H_
#define LOGOUTPACKET_H_

#include "../../util/MsgPacket.h"
#include "PacketMsgType.h"

namespace Dream {

// 登出请求包
    class LogoutReqPacket : public MsgPacket {

    public:
        LogoutReqPacket() :
                MsgPacket(MSG_LOGOUT_REQ) {

        }

        virtual ~LogoutReqPacket() = default;

        // ---------------------派生类重写开始---------------------
        // 无包体，无需覆盖父类方法
        // ---------------------派生类重写结束---------------------
    };

// 登出响应包
    class LogoutRespPacket : public MsgPacket {
    public:
        LogoutRespPacket() :
                MsgPacket(MSG_LOGOUT_RESP) {

        }

        virtual ~LogoutRespPacket() = default;

        // ---------------------派生类重写开始---------------------
        // 无包体，无需覆盖父类方法
        // ---------------------派生类重写结束---------------------
    };

} /* namespace Dream */

#endif /* LOGINREQPACKET_H_ */
