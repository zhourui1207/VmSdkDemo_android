/*
 * PushAlarmPacket.h
 *
 *  Created on: 2016年10月2日
 *      Author: zhourui
 *      推送报警包
 */

#ifndef PUSHALARMPACKET_H_
#define PUSHALARMPACKET_H_

#include "../../util/MsgPacket.h"
#include "../AlarmType.h"
#include "PacketMsgType.h"

namespace Dream {

// 推送报警包
    class PushAlarmPacket : public MsgPacket {
    public:
        PushAlarmPacket() :
                MsgPacket(MSG_ALARM_NOTIFY), _channelId(0), _alarmType(ALARM_TYPE_EXTERNEL),
                _parameter1(0), _parameter2(0) {

        }

        virtual ~PushAlarmPacket() = default;

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
            ENCODE_INT(pBody + pos, _alarmType, pos);
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
            DECODE_INT(pBody + pos, _alarmType, pos);
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
        unsigned _alarmType;
        unsigned _parameter1;
        unsigned _parameter2;
    };

} /* namespace Dream */

#endif /* DEPTREEPACKET_H_ */
