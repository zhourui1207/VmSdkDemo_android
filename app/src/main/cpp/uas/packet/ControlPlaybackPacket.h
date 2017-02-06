/*
 * ControlPlaybackPacket.h
 *
 *  Created on: 2016年9月28日
 *      Author: zhourui
 *      控制回放包
 *
 */

#ifndef CONTROLPLAYBACKPACKET_H_
#define CONTROLPLAYBACKPACKET_H_

#include "../../util/MsgPacket.h"
#include "PacketMsgType.h"

namespace Dream {

// 控制回放请求包
    class ControlPlaybackReqPacket : public MsgPacket {
    public:
        enum {
            MAIN_STREAM = 0,  // 主码流
            SUB_STREAM = 1  // 子码流
        };

        enum RECORD_TYPE {
            RECORD_TYPE_CENTER = 0,  // 中心录像
            RECORD_TYPE_DEVICE = 1  // 设备录像
        };

    public:
        ControlPlaybackReqPacket() :
                MsgPacket(MSG_CONTROL_PLAYBACK_REQ), _channelId(0), _subChannelId(
                MAIN_STREAM), _monitorSessionId(0), _recordType(RECORD_TYPE_CENTER), _controlId(0) {

        }

        virtual ~ControlPlaybackReqPacket() = default;

        // ---------------------派生类重写开始---------------------
        virtual std::size_t computeLength() const override {
            return MsgPacket::computeLength() + 5 * sizeof(int) + _fdId.length() + 1
                   + _action.length() + 1 + _param.length() + 1;
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
            ENCODE_INT(pBody + pos, _subChannelId, pos);
            ENCODE_INT(pBody + pos, _monitorSessionId, pos);
            ENCODE_INT(pBody + pos, _recordType, pos);
            ENCODE_INT(pBody + pos, _controlId, pos);
            ENCODE_STRING(pBody + pos, _action, pos);
            ENCODE_STRING(pBody + pos, _param, pos);
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
            DECODE_INT(pBody + pos, _subChannelId, pos);
            DECODE_INT(pBody + pos, _monitorSessionId, pos);
            DECODE_INT(pBody + pos, _recordType, pos);
            DECODE_INT(pBody + pos, _controlId, pos);
            DECODE_STRING(pBody + pos, _action, pos);
            DECODE_STRING(pBody + pos, _param, pos);

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
        unsigned _subChannelId;
        unsigned _monitorSessionId;
        unsigned _recordType;
        unsigned _controlId;
        std::string _action;
        std::string _param;
    };

// 控制回放响应包
    class ControlPlaybackRespPacket : public MsgPacket {
    public:
        enum {
            MAIN_STREAM = 0,  // 主码流
            SUB_STREAM = 1  // 子码流
        };

        enum RECORD_TYPE {
            RECORD_TYPE_CENTER = 0,  // 中心录像
            RECORD_TYPE_DEVICE = 1  // 设备录像
        };

    public:
        ControlPlaybackRespPacket() :
                MsgPacket(MSG_CONTROL_PLAYBACK_RESP), _channelId(0), _subChannelId(
                MAIN_STREAM), _monitorSessionId(0), _recordType(RECORD_TYPE_CENTER) {

        }

        virtual ~ControlPlaybackRespPacket() = default;

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
            ENCODE_INT(pBody + pos, _subChannelId, pos);
            ENCODE_INT(pBody + pos, _monitorSessionId, pos);
            ENCODE_INT(pBody + pos, _recordType, pos);
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
            DECODE_INT(pBody + pos, _subChannelId, pos);
            DECODE_INT(pBody + pos, _monitorSessionId, pos);
            DECODE_INT(pBody + pos, _recordType, pos);

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
        unsigned _subChannelId;
        unsigned _monitorSessionId;
        unsigned _recordType;
    };

} /* namespace Dream */

#endif /* LOGINREQPACKET_H_ */
