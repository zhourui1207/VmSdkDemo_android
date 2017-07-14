//
// Created by zhou rui on 2017/7/11.
//

#ifndef DREAM_UDTBASEPACKET_H
#define DREAM_UDTBASEPACKET_H

/**
 * UDT基类包，用来区分数据包和控制包
 */
#include <cstring>
#include "../../util/public/platform.h"

namespace Dream {

    class UDTBasePacket {

    public:
        static unsigned DEFAULT_MAX = UINT32_MAX;
        static unsigned SEQUENCE_NUMBER_MAX = INT32_MAX;

    public:
        UDTBasePacket() = delete;

        UDTBasePacket(bool control)
                : _control(control), _timestamp(0), _dstSocketId(0) {

        }

        UDTBasePacket(bool control, unsigned timestamp, unsigned dstSocketId)
                : _control(control), _timestamp(timestamp), _dstSocketId(dstSocketId) {

        }

        virtual ~UDTBasePacket() = default;

        virtual int encode(char *pBuf, std::size_t len) = 0;

        virtual int decode(const char *pBuf, std::size_t len) = 0;

        virtual std::size_t headerLength() {
            return sizeof(_timestamp) + sizeof(_dstSocketId);
        }

        virtual std::size_t totalLength() {
            return headerLength();
        }

        static bool decodeControlStatic(const unsigned char buf) {
            return (buf & 0x80) != 0;
        }

        bool decodeControl(const unsigned char buf) {
            _control = (buf & 0x80) != 0;
            return _control;
        }

        void encodeTimestamp(char *pBuf, int &pos) {
            ENCODE_INT(pBuf, _timestamp, pos);
        }

        void decodeTimestamp(const char *pBuf, int &pos) {
            DECODE_INT(pBuf, _timestamp, pos);
        }

        static bool decodeDstSocketIdStatic(const char *pBuf, std::size_t len, unsigned& dstSocketId) {
            if (len < 16) {
                return false;
            }
            int pos = 0;
            DECODE_INT(pBuf + 12, dstSocketId, pos);
            return true;
        }

        void encodeDstSocketId(char *pBuf, int &pos) {
            ENCODE_INT(pBuf, _dstSocketId, pos);
        }

        int decodeDstSocketId(const char *pBuf, int &pos) {
            DECODE_INT(pBuf, _dstSocketId, pos);
        }

        void setControl(bool control) {
            _control = control;
        }

        bool isControl() const {
            return _control;
        }

        void setTimestamp(unsigned timestamp) {
            _timestamp = timestamp;
        }

        unsigned timestamp() const {
            return _timestamp;
        }

        void setDstSocketId(unsigned dstSocketId) {
            _dstSocketId = dstSocketId;
        }

        unsigned dstSocketId() const {
            return _dstSocketId;
        }

    private:
        bool _control;  // 是否是控制包
        unsigned _timestamp;  // 时间戳
        unsigned _dstSocketId;  // 目的socket id
    };

}

#endif //VMSDKDEMO_ANDROID_UDTBASEPACKET_H
