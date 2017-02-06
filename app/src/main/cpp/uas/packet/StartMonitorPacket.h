/*
 * StartMonitorPacket.h
 *
 *  Created on: 2016年9月28日
 *      Author: zhourui
 *      开始预览包
 *
 */

#ifndef STARTMONITORPACKET_H_
#define STARTMONITORPACKET_H_

#include "../../util/MsgPacket.h"
#include "PacketMsgType.h"

namespace Dream {

// 开始预览请求包
    class StartMonitorReqPacket : public MsgPacket {
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

        enum {
            MAIN_STREAM = 0,  // 主码流
            SUB_STREAM = 1  // 子码流
        };

    public:
        StartMonitorReqPacket() :
                MsgPacket(MSG_START_MONITOR_REQ), _channelId(0), _subChannelId(
                MAIN_STREAM), _monitorSessionId(0), _activeFlag(
                ACTIVE_FLAG_NO_ACTIVE), _videoPort(0), _videoTransMode(
                TRANS_MODE_TCP), _audioPort(0), _audioTransMode(TRANS_MODE_TCP) {

        }

        virtual ~StartMonitorReqPacket() = default;

        // ---------------------派生类重写开始---------------------
        virtual std::size_t computeLength() const override {
            return MsgPacket::computeLength() + 8 * sizeof(int) + _fdId.length() + 1
                   + _videoIp.length() + 1 + _audioIp.length() + 1;
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
            ENCODE_INT(pBody + pos, _activeFlag, pos);
            ENCODE_STRING(pBody + pos, _videoIp, pos);
            ENCODE_INT(pBody + pos, _videoPort, pos);
            ENCODE_INT(pBody + pos, _videoTransMode, pos);
            ENCODE_STRING(pBody + pos, _audioIp, pos);
            ENCODE_INT(pBody + pos, _audioPort, pos);
            ENCODE_INT(pBody + pos, _audioTransMode, pos);
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
            DECODE_INT(pBody + pos, _activeFlag, pos);
            DECODE_STRING(pBody + pos, _videoIp, pos);
            DECODE_INT(pBody + pos, _videoPort, pos);
            DECODE_INT(pBody + pos, _videoTransMode, pos);
            DECODE_STRING(pBody + pos, _audioIp, pos);
            DECODE_INT(pBody + pos, _audioPort, pos);
            DECODE_INT(pBody + pos, _audioTransMode, pos);

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
        unsigned _activeFlag;
        std::string _videoIp;
        unsigned _videoPort;
        unsigned _videoTransMode;
        std::string _audioIp;
        unsigned _audioPort;
        unsigned _audioTransMode;
    };

// 开始预览响应包
    class StartMonitorRespPacket : public MsgPacket {
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

        enum {
            MAIN_STREAM = 0,  // 主码流
            SUB_STREAM = 1  // 子码流
        };

    public:
        StartMonitorRespPacket() :
                MsgPacket(MSG_START_MONITOR_RESP), _channelId(0), _subChannelId(
                MAIN_STREAM), _monitorSessionId(0), _activeFlag(
                ACTIVE_FLAG_NO_ACTIVE), _videoPort(0), _videoTransMode(
                TRANS_MODE_TCP), _audioPort(0), _audioTransMode(TRANS_MODE_TCP) {

        }

        virtual ~StartMonitorRespPacket() = default;

        // ---------------------派生类重写开始---------------------
        virtual std::size_t computeLength() const override {
            return MsgPacket::computeLength() + 8 * sizeof(int) + _fdId.length() + 1
                   + _videoIp.length() + 1 + _audioIp.length() + 1;
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
            ENCODE_INT(pBody + pos, _activeFlag, pos);
            ENCODE_STRING(pBody + pos, _videoIp, pos);
            ENCODE_INT(pBody + pos, _videoPort, pos);
            ENCODE_INT(pBody + pos, _videoTransMode, pos);
            ENCODE_STRING(pBody + pos, _audioIp, pos);
            ENCODE_INT(pBody + pos, _audioPort, pos);
            ENCODE_INT(pBody + pos, _audioTransMode, pos);
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
            DECODE_INT(pBody + pos, _activeFlag, pos);
            DECODE_STRING(pBody + pos, _videoIp, pos);
            DECODE_INT(pBody + pos, _videoPort, pos);
            DECODE_INT(pBody + pos, _videoTransMode, pos);
            DECODE_STRING(pBody + pos, _audioIp, pos);
            DECODE_INT(pBody + pos, _audioPort, pos);
            DECODE_INT(pBody + pos, _audioTransMode, pos);

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
        unsigned _activeFlag;
        std::string _videoIp;
        unsigned _videoPort;
        unsigned _videoTransMode;
        std::string _audioIp;
        unsigned _audioPort;
        unsigned _audioTransMode;
    };

} /* namespace Dream */

#endif /* LOGINREQPACKET_H_ */
