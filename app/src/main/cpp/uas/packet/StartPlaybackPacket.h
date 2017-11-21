/*
 * StartPlaybackPacket.h
 *
 *  Created on: 2016年9月28日
 *      Author: zhourui
 *      开始回放包
 *
 */

#ifndef STARTPLAYBACKPACKET_H_
#define STARTPLAYBACKPACKET_H_

#include "../../util/MsgPacket.h"
#include "PacketMsgType.h"

namespace Dream {

// 开始预览请求包
    class StartPlaybackReqPacket : public MsgPacket {
    public:
        enum RECORD_TYPE {
            RECORD_TYPE_CENTER = 0,  // 中心录像
            RECORD_TYPE_DEVICE = 1  // 设备录像
        };

        enum FLAG {
            FLAG_PLAYBACK = 0,  // 回放
            FLAG_DOWNLOAD = 1  // 下载
        };

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
        StartPlaybackReqPacket() :
                MsgPacket(MSG_START_PLAYBACK_REQ), _channelId(0), _subChannelId(
                MAIN_STREAM), _monitorSessionId(0), _recordType(RECORD_TYPE_CENTER), _flag(
                FLAG_PLAYBACK), _startTime(0), _endTime(0), _activeFlag(
                ACTIVE_FLAG_NO_ACTIVE), _videoPort(0), _videoTransMode(
                TRANS_MODE_TCP), _audioPort(0), _audioTransMode(TRANS_MODE_TCP) {

        }

        virtual ~StartPlaybackReqPacket() = default;

        // ---------------------派生类重写开始---------------------
        virtual std::size_t computeLength() const override {
            return MsgPacket::computeLength() + 12 * sizeof(int) + _fdId.length() + 1
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
            ENCODE_INT32(pBody + pos, _channelId, pos);
            ENCODE_INT32(pBody + pos, _subChannelId, pos);
            ENCODE_INT32(pBody + pos, _monitorSessionId, pos);
            ENCODE_INT32(pBody + pos, _recordType, pos);
            ENCODE_INT32(pBody + pos, _flag, pos);
            ENCODE_INT32(pBody + pos, _startTime, pos);
            ENCODE_INT32(pBody + pos, _endTime, pos);
            ENCODE_INT32(pBody + pos, _activeFlag, pos);
            ENCODE_STRING(pBody + pos, _videoIp, pos);
            ENCODE_INT32(pBody + pos, _videoPort, pos);
            ENCODE_INT32(pBody + pos, _videoTransMode, pos);
            ENCODE_STRING(pBody + pos, _audioIp, pos);
            ENCODE_INT32(pBody + pos, _audioPort, pos);
            ENCODE_INT32(pBody + pos, _audioTransMode, pos);
            return dataLength;
        }

        virtual int decode(const char *pBuf, std::size_t len) override {
            int usedLen = MsgPacket::decode(pBuf, len);
            if (usedLen < 0) {
                return -1;
            }

            char *pBody = (char *) (pBuf + usedLen);

            int pos = 0;
            DECODE_STRING(pBody + pos, _fdId, pos);
            DECODE_INT32(pBody + pos, _channelId, pos);
            DECODE_INT32(pBody + pos, _subChannelId, pos);
            DECODE_INT32(pBody + pos, _monitorSessionId, pos);
            DECODE_INT32(pBody + pos, _recordType, pos);
            DECODE_INT32(pBody + pos, _flag, pos);
            DECODE_INT32(pBody + pos, _startTime, pos);
            DECODE_INT32(pBody + pos, _endTime, pos);
            DECODE_INT32(pBody + pos, _activeFlag, pos);
            DECODE_STRING(pBody + pos, _videoIp, pos);
            DECODE_INT32(pBody + pos, _videoPort, pos);
            DECODE_INT32(pBody + pos, _videoTransMode, pos);
            DECODE_STRING(pBody + pos, _audioIp, pos);
            DECODE_INT32(pBody + pos, _audioPort, pos);
            DECODE_INT32(pBody + pos, _audioTransMode, pos);

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
        unsigned _flag;
        unsigned _startTime;
        unsigned _endTime;
        unsigned _activeFlag;
        std::string _videoIp;
        unsigned _videoPort;
        unsigned _videoTransMode;
        std::string _audioIp;
        unsigned _audioPort;
        unsigned _audioTransMode;
    };

// 开始回放响应包
    class StartPlaybackRespPacket : public MsgPacket {
    public:
        enum RECORD_TYPE {
            RECORD_TYPE_CENTER = 0,  // 中心录像
            RECORD_TYPE_DEVICE = 1  // 设备录像
        };

        enum FLAG {
            FLAG_PLAYBACK = 0,  // 回放
            FLAG_DOWNLOAD = 1  // 下载
        };

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
        StartPlaybackRespPacket() :
                MsgPacket(MSG_START_PLAYBACK_RESP), _channelId(0), _subChannelId(
                MAIN_STREAM), _monitorSessionId(0), _recordType(RECORD_TYPE_CENTER), _flag(
                FLAG_PLAYBACK), _startTime(0), _endTime(0), _activeFlag(
                ACTIVE_FLAG_NO_ACTIVE), _videoPort(0), _videoTransMode(
                TRANS_MODE_TCP), _audioPort(0), _audioTransMode(TRANS_MODE_TCP) {

        }

        virtual ~StartPlaybackRespPacket() {

        }

        // ---------------------派生类重写开始---------------------
        virtual std::size_t computeLength() const override {
            return MsgPacket::computeLength() + 12 * sizeof(int) + _fdId.length() + 1
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
            ENCODE_INT32(pBody + pos, _channelId, pos);
            ENCODE_INT32(pBody + pos, _subChannelId, pos);
            ENCODE_INT32(pBody + pos, _monitorSessionId, pos);
            ENCODE_INT32(pBody + pos, _recordType, pos);
            ENCODE_INT32(pBody + pos, _flag, pos);
            ENCODE_INT32(pBody + pos, _startTime, pos);
            ENCODE_INT32(pBody + pos, _endTime, pos);
            ENCODE_INT32(pBody + pos, _activeFlag, pos);
            ENCODE_STRING(pBody + pos, _videoIp, pos);
            ENCODE_INT32(pBody + pos, _videoPort, pos);
            ENCODE_INT32(pBody + pos, _videoTransMode, pos);
            ENCODE_STRING(pBody + pos, _audioIp, pos);
            ENCODE_INT32(pBody + pos, _audioPort, pos);
            ENCODE_INT32(pBody + pos, _audioTransMode, pos);
            return dataLength;
        }

        virtual int decode(const char *pBuf, std::size_t len) override {
            int usedLen = MsgPacket::decode(pBuf, len);
            if (usedLen < 0) {
                return -1;
            }

            char *pBody = (char *) (pBuf + usedLen);

            int pos = 0;
            DECODE_STRING(pBody + pos, _fdId, pos);
            DECODE_INT32(pBody + pos, _channelId, pos);
            DECODE_INT32(pBody + pos, _subChannelId, pos);
            DECODE_INT32(pBody + pos, _monitorSessionId, pos);
            DECODE_INT32(pBody + pos, _recordType, pos);
            DECODE_INT32(pBody + pos, _flag, pos);
            DECODE_INT32(pBody + pos, _startTime, pos);
            DECODE_INT32(pBody + pos, _endTime, pos);
            DECODE_INT32(pBody + pos, _activeFlag, pos);
            DECODE_STRING(pBody + pos, _videoIp, pos);
            DECODE_INT32(pBody + pos, _videoPort, pos);
            DECODE_INT32(pBody + pos, _videoTransMode, pos);
            DECODE_STRING(pBody + pos, _audioIp, pos);
            DECODE_INT32(pBody + pos, _audioPort, pos);
            DECODE_INT32(pBody + pos, _audioTransMode, pos);

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
        unsigned _flag;
        unsigned _startTime;
        unsigned _endTime;
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
