//
// Created by zhou rui on 2017/7/12.
//

#ifndef DREAM_UDTMDRPACKET_H
#define DREAM_UDTMDRPACKET_H

/**
 * udt消息丢失包
 */
#include "UDTControlPacket.h"

namespace Dream {

    class UDTMDRPacket : public UDTControlPacket {

    public:
        UDTMDRPacket(unsigned msgId, unsigned firstSeqNumber, unsigned lastSeqNumber) :
                UDTControlPacket(MDR, 0, msgId), _firstSeqNumber(firstSeqNumber),
                _lastSeqNumber(lastSeqNumber) {

        }

        virtual ~UDTMDRPacket() = default;

        virtual int encode(char *pBuf, std::size_t len) override {
            if (len < totalLength()) {
                return -1;
            }

            int encodePos = UDTControlPacket::encode(pBuf, len);

            if (encodePos >= 0) {
                ENCODE_INT(pBuf + encodePos, _firstSeqNumber, encodePos);
                ENCODE_INT(pBuf + encodePos, _lastSeqNumber, encodePos);
            }

            return encodePos;
        }

        virtual int decode(const char *pBuf, std::size_t len) override {
            if (len < headerLength()) {
                return -1;
            }

            int decodePos = UDTControlPacket::decode(pBuf, len);

            if (decodePos >= 0) {
                DECODE_INT(pBuf + decodePos, _firstSeqNumber, decodePos);
                DECODE_INT(pBuf + decodePos, _lastSeqNumber, decodePos);
            }

            return decodePos;
        }

        virtual std::size_t headerLength() override {
            return UDTControlPacket::headerLength() + sizeof(_firstSeqNumber) + sizeof(_lastSeqNumber);
        }

        virtual std::size_t totalLength() override {
            return headerLength();
        }

    private:
        unsigned _firstSeqNumber;  // 第一个丢弃的序列号
        unsigned _lastSeqNumber;  // 最后一个丢弃的序列号

    };

}

#endif //VMSDKDEMO_ANDROID_UDTSYNPACKET_H
