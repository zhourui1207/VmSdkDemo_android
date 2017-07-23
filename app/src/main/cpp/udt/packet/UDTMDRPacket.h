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
        UDTMDRPacket(uint32_t msgId, int32_t firstSeqNumber, int32_t lastSeqNumber) :
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
                ENCODE_INT32(pBuf + encodePos, _firstSeqNumber, encodePos);
                ENCODE_INT32(pBuf + encodePos, _lastSeqNumber, encodePos);
            }

            return encodePos;
        }

        virtual int decode(const char *pBuf, std::size_t len) override {
            if (len < headerLength()) {
                return -1;
            }

            int decodePos = UDTControlPacket::decode(pBuf, len);

            if (decodePos >= 0) {
                DECODE_INT32(pBuf + decodePos, _firstSeqNumber, decodePos);
                DECODE_INT32(pBuf + decodePos, _lastSeqNumber, decodePos);
            }

            return decodePos;
        }

        virtual static std::size_t headerLength() {
            return UDTControlPacket::headerLength() + sizeof(int32_t) + sizeof(int32_t);
        }

        virtual std::size_t totalLength() override {
            return headerLength();
        }

    private:
        int32_t _firstSeqNumber;  // 第一个丢弃的序列号
        int32_t _lastSeqNumber;  // 最后一个丢弃的序列号

    };

}

#endif //VMSDKDEMO_ANDROID_UDTSYNPACKET_H
