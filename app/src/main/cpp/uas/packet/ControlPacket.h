/*
 * ControlPacket.h
 *
 *  Created on: 2016年9月28日
 *      Author: zhourui
 *      控制包
 *
 */

#ifndef CONTROLPACKET_H_
#define CONTROLPACKET_H_

#include "../../util/MsgPacket.h"
#include "../ControlType.h"
#include "PacketMsgType.h"

namespace Dream {

// 控制请求包
    class ControlReqPacket : public MsgPacket {
    public:
        ControlReqPacket() :
                MsgPacket(MSG_CONTROL_REQ), _channelId(0), _controlType(
                CONTROL_TYPE_PTZ_STOP), _parameter1(0), _parameter2(0) {

        }

        virtual ~ControlReqPacket() = default;

        // ---------------------派生类重写开始---------------------
        virtual std::size_t computeLength() const override {
            return MsgPacket::computeLength() + 4 * sizeof(int) + _fdId.length() + 1;
        }

        virtual int encode(char *pBuf, std::size_t len) override {
            std::size_t dataLength = computeLength();
            if (len < dataLength) {
                return -1;
            }
            int usedLen = MsgPacket::encode(pBuf, len);
            if (usedLen < 0) {
                return -1;
            }

            char *pBody = pBuf + usedLen;

            int pos = 0;
            ENCODE_STRING(pBody + pos, _fdId, pos);
            ENCODE_INT(pBody + pos, _channelId, pos);
            ENCODE_INT(pBody + pos, _controlType, pos);
            ENCODE_INT(pBody + pos, _parameter1, pos);
            ENCODE_INT(pBody + pos, _parameter2, pos);
            return dataLength;
        }

        virtual int decode(char *pBuf, std::size_t len) override {
            int usedLen = MsgPacket::decode(pBuf, len);
            if (usedLen < 0) {
                return -1;
            }

            char *pBody = pBuf + usedLen;

            int pos = 0;
            DECODE_STRING(pBody + pos, _fdId, pos);
            DECODE_INT(pBody + pos, _channelId, pos);
            DECODE_INT(pBody + pos, _controlType, pos);
            DECODE_INT(pBody + pos, _parameter1, pos);
            DECODE_INT(pBody + pos, _parameter2, pos);

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
        unsigned _controlType;
        unsigned _parameter1;
        unsigned _parameter2;
    };

// 控制响应包
    class ControlRespPacket : public MsgPacket {
    public:
        ControlRespPacket() :
                MsgPacket(MSG_CONTROL_RESP) {

        }

        virtual ~ControlRespPacket() = default;

        // ---------------------派生类重写开始---------------------
        // ---------------------派生类重写结束---------------------
    };

} /* namespace Dream */

#endif /* LOGINREQPACKET_H_ */
