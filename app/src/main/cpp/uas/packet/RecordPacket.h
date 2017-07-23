/*
 * RecordPacket.h
 *
 *  Created on: 2016年10月2日
 *      Author: zhourui
 *      录像包
 */

#ifndef RECORDPACKET_H_
#define RECORDPACKET_H_

#include "../../util/MsgPacket.h"
#include "PacketMsgType.h"

namespace Dream {

    class RecordItem {
    public:
        RecordItem() :
                _beginTime(0), _endTime(0) {
        }

        virtual ~RecordItem() = default;

        std::size_t computeLength() const {
            return 2 * sizeof(int) + _playbackUrl.length() + 1 + _downloadUrl.length() + 1;
        }

    public:
        unsigned _beginTime;
        unsigned _endTime;
        std::string _playbackUrl;
        std::string _downloadUrl;
    };

    using RecordList = std::vector<RecordItem>;

// 获取录像请求包
    class GetRecordReqPacket : public MsgPacket {
    public:
        enum RECORD_TYPE {
            RECORD_TYPE_CENTER = 0,  // 中心录像
            RECORD_TYPE_DEVICE = 1  // 设备录像
        };

        enum {
            MAIN_STREAM = 0,  // 主码流
            SUB_STREAM = 1  // 子码流
        };

        enum {
            NONE_NAT = 0,  // 不使用nat穿越
            IS_NAT = 1  // 使用nat穿越
        };

    public:
        GetRecordReqPacket() :
                MsgPacket(MSG_GET_RECORD_REQ), _channelId(0), _subChannelId(
                MAIN_STREAM), _beginTime(0), _endTime(0), _isNat(NONE_NAT), _recordType(
                RECORD_TYPE_CENTER) {

        }

        virtual ~GetRecordReqPacket() = default;

        // ---------------------派生类重写开始---------------------
        virtual std::size_t computeLength() const override {
            return MsgPacket::computeLength() + 6 * sizeof(int) + _fdId.length() + 1 +
                   _clientIp.length() + 1;
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
            ENCODE_INT32(pBody + pos, _beginTime, pos);
            ENCODE_INT32(pBody + pos, _endTime, pos);
            ENCODE_INT32(pBody + pos, _isNat, pos);
            ENCODE_STRING(pBody + pos, _clientIp, pos);
            ENCODE_INT32(pBody + pos, _recordType, pos);

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
            DECODE_INT32(pBody + pos, _channelId, pos);
            DECODE_INT32(pBody + pos, _subChannelId, pos);
            DECODE_INT32(pBody + pos, _beginTime, pos);
            DECODE_INT32(pBody + pos, _endTime, pos);
            DECODE_INT32(pBody + pos, _isNat, pos);
            DECODE_STRING(pBody + pos, _clientIp, pos);
            DECODE_INT32(pBody + pos, _recordType, pos);

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
        int _subChannelId;
        unsigned _beginTime;
        unsigned _endTime;
        unsigned _isNat;
        std::string _clientIp;
        unsigned _recordType;
    };

// 获取录像响应包
    class GetRecordRespPacket : public MsgPacket {
    public:
        GetRecordRespPacket() :
                MsgPacket(MSG_GET_RECORD_RESP), _rightFlags(0), _recordCounts(0) {

        }

        virtual ~GetRecordRespPacket() = default;

        // ---------------------派生类重写开始---------------------
        virtual std::size_t computeLength() const override {
            std::size_t len = 0;
            for (std::size_t i = 0; i < _records.size(); ++i) {
                len += _records[i].computeLength();
            }
            return MsgPacket::computeLength() + 2 * sizeof(int) + len;
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
            ENCODE_INT32(pBody + pos, _rightFlags, pos);
            _recordCounts = _records.size();
            ENCODE_INT32(pBody + pos, _recordCounts, pos);
            for (unsigned i = 0; i < _recordCounts; ++i) {
                ENCODE_INT32(pBody + pos, _records[i]._beginTime, pos);
                ENCODE_INT32(pBody + pos, _records[i]._endTime, pos);
                ENCODE_STRING(pBody + pos, _records[i]._playbackUrl, pos);
                ENCODE_STRING(pBody + pos, _records[i]._downloadUrl, pos);
            }

            return dataLength;
        }

        virtual int decode(char *pBuf, std::size_t len) override {
            int usedLen = MsgPacket::decode(pBuf, len);
            if (usedLen < 0) {
                return -1;
            }

            char *pBody = pBuf + usedLen;

            int pos = 0;
            DECODE_INT32(pBody + pos, _rightFlags, pos);
            DECODE_INT32(pBody + pos, _recordCounts, pos);
            for (unsigned i = 0; i < _recordCounts; ++i) {
                RecordItem item;
                DECODE_INT32(pBody + pos, item._beginTime, pos);
                DECODE_INT32(pBody + pos, item._endTime, pos);
                DECODE_STRING(pBody + pos, item._playbackUrl, pos);
                DECODE_STRING(pBody + pos, item._downloadUrl, pos);
                _records.push_back(item);
            }

            // 包长和实际长度不符
            if (computeLength() != length()) {
                return -1;
            }
            return length();
        }
        // ---------------------派生类重写结束---------------------

    public:
        unsigned _rightFlags;  // 权限标志
        unsigned _recordCounts;  // 数量
        RecordList _records;  // 录像列表
    };

} /* namespace Dream */

#endif /* DEPTREEPACKET_H_ */
