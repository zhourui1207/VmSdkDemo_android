/*
 * ActivePacket.h
 *
 *  Created on: 2016年9月28日
 *      Author: zhourui
 *      保活包
 *
 */

#ifndef DREAM_AUTHRESPPACKET_H_
#define DREAM_AUTHRESPPACKET_H_

#include "../../util/StreamPacket.h"
#include "PacketStreamType.h"

namespace Dream {

    class AuthRespPacket : public StreamPacket {

    public:
        AuthRespPacket() :
                StreamPacket(MSG_AUTH_RESP) {

        }

        virtual ~AuthRespPacket() {

        }

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
            DECODE_INT32(pBuf + pos, _length, pos);
            DECODE_INT32(pBuf + pos, _msgType, pos);
            DECODE_STRING(pBuf + pos, _key, pos);
            return pos;
        }
        // ---------------------派生类重写结束---------------------

        std::string key() const {
            return _key;
        }

    private:
        std::string _key;
    };

} /* namespace Dream */

#endif /* LOGINREQPACKET_H_ */
