/*
 * AlarmPacket.h
 *
 *  Created on: 2016年10月2日
 *      Author: zhourui
 *      报警包
 */

#ifndef ALARMPACKET_H_
#define ALARMPACKET_H_

#include "../../util/MsgPacket.h"
#include "PacketMsgType.h"

namespace Dream {

    class AlarmItem {
    public:
        AlarmItem() :
                _channelId(0), _channelBigType(0), _alarmCode(0), _alarmType(0) {
        }

        virtual ~AlarmItem() = default;

        std::size_t computeLength() const {
            return 4 * sizeof(int) + _alarmId.length() + 1 + _fdId.length() + 1
                   + _fdName.length() + 1 + _channelName.length() + 1 + _alarmTime.length()
                   + 1 + _alarmName.length() + 1 + _alarmSubName.length() + 1 + _photoUrl.length() +
                   1;
        }

    public:
        std::string _alarmId;
        std::string _fdId;
        std::string _fdName;
        int _channelId;
        std::string _channelName;
        unsigned _channelBigType;
        std::string _alarmTime;
        unsigned _alarmCode;
        std::string _alarmName;
        std::string _alarmSubName;
        unsigned _alarmType;
        std::string _photoUrl;
    };

    using AlarmList = std::vector<AlarmItem>;

// 获取报警请求包
    class GetAlarmReqPacket : public MsgPacket {
    public:
        enum CHANNEL_BIG_TYPE {
            CHANNEL_BIG_TYPE_VIDEO_IN = 1,  // 视频输入
            CHANNEL_BIG_TYPE_ALARM_IN = 5  // 报警输入
        };

    public:
        GetAlarmReqPacket() :
                MsgPacket(MSG_GET_ALARM_REQ), _channelId(0),
                _channelBigType(CHANNEL_BIG_TYPE_VIDEO_IN) {

        }

        virtual ~GetAlarmReqPacket() = default;

        // ---------------------派生类重写开始---------------------
        virtual std::size_t computeLength() const override {
            return MsgPacket::computeLength() + 2 * sizeof(int) + _fdId.length() + 1
                   + _beginTime.length() + 1 + _endTime.length() + 1;
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
            ENCODE_INT(pBody + pos, _channelBigType, pos);
            ENCODE_STRING(pBody + pos, _beginTime, pos);
            ENCODE_STRING(pBody + pos, _endTime, pos);

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
            DECODE_INT(pBody + pos, _channelBigType, pos);
            DECODE_STRING(pBody + pos, _beginTime, pos);
            DECODE_STRING(pBody + pos, _endTime, pos);

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
        unsigned _channelBigType;
        std::string _beginTime;
        std::string _endTime;
    };

// 获取报警响应包
    class GetAlarmRespPacket : public MsgPacket {
    public:
        GetAlarmRespPacket() :
                MsgPacket(MSG_GET_ALARM_RESP), _alarmCounts(0) {

        }

        virtual ~GetAlarmRespPacket() = default;

        // ---------------------派生类重写开始---------------------
        virtual std::size_t computeLength() const override {
            std::size_t len = 0;
            for (std::size_t i = 0; i < _alarms.size(); ++i) {
                len += _alarms[i].computeLength();
            }
            return MsgPacket::computeLength() + sizeof(int) + len;
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
            _alarmCounts = _alarms.size();
            ENCODE_INT(pBody + pos, _alarmCounts, pos);
            for (unsigned i = 0; i < _alarmCounts; ++i) {
                ENCODE_STRING(pBody + pos, _alarms[i]._alarmId, pos);
                ENCODE_STRING(pBody + pos, _alarms[i]._fdId, pos);
                ENCODE_STRING(pBody + pos, _alarms[i]._fdName, pos);
                ENCODE_INT(pBody + pos, _alarms[i]._channelId, pos);
                ENCODE_STRING(pBody + pos, _alarms[i]._channelName, pos);
                ENCODE_INT(pBody + pos, _alarms[i]._channelBigType, pos);
                ENCODE_STRING(pBody + pos, _alarms[i]._alarmTime, pos);
                ENCODE_INT(pBody + pos, _alarms[i]._alarmCode, pos);
                ENCODE_STRING(pBody + pos, _alarms[i]._alarmName, pos);
                ENCODE_STRING(pBody + pos, _alarms[i]._alarmSubName, pos);
                ENCODE_INT(pBody + pos, _alarms[i]._alarmType, pos);
                ENCODE_STRING(pBody + pos, _alarms[i]._photoUrl, pos);
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
            DECODE_INT(pBody + pos, _alarmCounts, pos);
            for (unsigned i = 0; i < _alarmCounts; ++i) {
                AlarmItem item;
                DECODE_STRING(pBody + pos, item._alarmId, pos);
                DECODE_STRING(pBody + pos, item._fdId, pos);
                DECODE_STRING(pBody + pos, item._fdName, pos);
                DECODE_INT(pBody + pos, item._channelId, pos);
                DECODE_STRING(pBody + pos, item._channelName, pos);
                DECODE_INT(pBody + pos, item._channelBigType, pos);
                DECODE_STRING(pBody + pos, item._alarmTime, pos);
                DECODE_INT(pBody + pos, item._alarmCode, pos);
                DECODE_STRING(pBody + pos, item._alarmName, pos);
                DECODE_STRING(pBody + pos, item._alarmSubName, pos);
                DECODE_INT(pBody + pos, item._alarmType, pos);
                DECODE_STRING(pBody + pos, item._photoUrl, pos);
                _alarms.push_back(item);
            }

            // 包长和实际长度不符
            if (computeLength() != length()) {
                return -1;
            }
            return length();
        }
        // ---------------------派生类重写结束---------------------

    public:
        unsigned _alarmCounts;  // 数量
        AlarmList _alarms;  // 报警列表
    };

} /* namespace Dream */

#endif /* DEPTREEPACKET_H_ */
