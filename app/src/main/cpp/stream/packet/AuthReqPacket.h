/*
 * ActivePacket.h
 *
 *  Created on: 2016年9月28日
 *      Author: zhourui
 *      保活包
 *
 */

#ifndef DREAM_AUTHREQPACKET_H_
#define DREAM_AUTHREQPACKET_H_

#include "../../util/StreamPacket.h"
#include "PacketStreamType.h"

namespace Dream {

    class AuthReqPacket : public StreamPacket {

    public:
        AuthReqPacket(const std::string &key) :
                StreamPacket(MSG_AUTH_REQ), _key(key) {

        }

        virtual ~AuthReqPacket() = default;

        // ---------------------派生类重写开始---------------------
        virtual std::size_t computeLength() const override {
            return HEADER_LENGTH + _key.length() + 1;
        }

        virtual int encode(char *pBuf, std::size_t len) override {
            if (len < HEADER_LENGTH) {
                return -1;
            }

            int pos = 0;
            ENCODE_INT32(pBuf + pos, _length, pos);
            ENCODE_INT32(pBuf + pos, _msgType, pos);
            ENCODE_STRING(pBuf + pos, _key, pos);
            return pos;
        }

        virtual int decode(const char *pBuf, std::size_t len) override {
            if (len < HEADER_LENGTH) {
                return -1;
            }

            int pos = 0;
//            DECODE_INT32(pBuf + pos, _length, pos);
//            DECODE_INT32(pBuf + pos, _msgType, pos);
            DECODE_STRING(pBuf + pos, _key, pos);
            return pos;
        }
        // ---------------------派生类重写结束---------------------
    private:
        std::string _key;
    };

} /* namespace Dream */

#endif /* LOGINREQPACKET_H_ */
