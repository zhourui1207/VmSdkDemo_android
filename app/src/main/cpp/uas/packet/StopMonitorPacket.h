/*
 * StopMonitorPacket.h
 *
 *  Created on: 2016年9月28日
 *      Author: zhourui
 *      停止预览包
 *
 */

#ifndef STOPMONITORPACKET_H_
#define STOPMONITORPACKET_H_

#include "../../util/MsgPacket.h"
#include "PacketMsgType.h"

namespace Dream {

// 停止预览请求包
    class StopMonitorReqPacket : public MsgPacket {
    public:
        enum {
            MAIN_STREAM = 0,  // 主码流
            SUB_STREAM = 1  // 子码流
        };

    public:
        StopMonitorReqPacket() :
                MsgPacket(MSG_STOP_MONITOR_REQ), _channelId(0), _subChannelId(
                MAIN_STREAM), _monitorSessionId(0) {

        }

        virtual ~StopMonitorReqPacket() = default;

        // ---------------------派生类重写开始---------------------
        virtual std::size_t computeLength() const override {
            return MsgPacket::computeLength() + 3 * sizeof(int) + _fdId.length() + 1;
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
    };

// 停止预览响应包
    class StopMonitorRespPacket : public MsgPacket {
    public:
        enum {
            MAIN_STREAM = 0,  // 主码流
            SUB_STREAM = 1  // 子码流
        };

    public:
        StopMonitorRespPacket() :
                MsgPacket(MSG_STOP_MONITOR_RESP), _channelId(0), _subChannelId(
                MAIN_STREAM), _monitorSessionId(0) {

        }

        virtual ~StopMonitorRespPacket() = default;

        // ---------------------派生类重写开始---------------------
        virtual std::size_t computeLength() const override {
            return MsgPacket::computeLength() + 3 * sizeof(int) + _fdId.length() + 1;
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
    };

} /* namespace Dream */

#endif /* LOGINREQPACKET_H_ */
