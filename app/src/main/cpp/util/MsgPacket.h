/*
 * MessagePacket.h
 *
 *  Created on: 2016年9月27日
 *      Author: zhourui
 */


#ifndef MSGPACKET_H_
#define MSGPACKET_H_

#include "BasePacket.h"

namespace Dream {

    class MsgPacket : public BasePacket {
    public:
        static const unsigned HEADER_LENGTH = 4 * sizeof(int);
        static const unsigned BODY_MAX_SIZE = 65515;

        MsgPacket()
                : _length(HEADER_LENGTH), _msgType(0), _seqNumber(0), _status(0) {
        }

        MsgPacket(unsigned msgType)
                : _length(HEADER_LENGTH), _msgType(msgType), _seqNumber(0), _status(0) {

        }

        virtual ~MsgPacket() {
        }

        // ---------------------派生类重写开始---------------------
        virtual std::size_t computeLength() const override {
            return HEADER_LENGTH;
        }

        virtual int encode(char *pBuf, std::size_t len) override {
            if (len < HEADER_LENGTH) {
                return -1;
            }

            int pos = 0;
            ENCODE_INT(pBuf + pos, _length, pos);
            ENCODE_INT(pBuf + pos, _msgType, pos);
            ENCODE_INT(pBuf + pos, _seqNumber, pos);
            ENCODE_INT(pBuf + pos, _status, pos);
            return HEADER_LENGTH;
        }

        virtual int decode(char *pBuf, std::size_t len) override {
            if (len < HEADER_LENGTH) {
                return -1;
            }

            int pos = 0;
            DECODE_INT(pBuf + pos, _length, pos);
            DECODE_INT(pBuf + pos, _msgType, pos);
            DECODE_INT(pBuf + pos, _seqNumber, pos);
            DECODE_INT(pBuf + pos, _status, pos);
            return HEADER_LENGTH;
        }
        // ---------------------派生类重写结束---------------------

        void length(std::size_t len) override {
            _length = len;
        }

        unsigned length() const {
            return _length;
        }

        unsigned msgType() const {
            return _msgType;
        }

        void msgType(unsigned msgType) {
            _msgType = msgType;
        }

        unsigned seqNumber() const {
            return _seqNumber;
        }

        void seqNumber(unsigned seqNumber) {
            _seqNumber = seqNumber;
        }

        unsigned status() const {
            return _status;
        }

        void status(unsigned status) {
            _status = status;
        }

    private:
        /**
         * 消息结构定义：包总长＋消息类型＋序列号＋状态＋数据
         */
        unsigned _length;  // 包总长
        unsigned _msgType;  // 消息类型
        unsigned _seqNumber;  // 序列号
        unsigned _status;  // 状态
    };

} /* namespace Dream */

#endif /* MESSAGEPACKET_H_ */
