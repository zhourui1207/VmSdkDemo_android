//
// Created by zhou rui on 2017/7/12.
//

#ifndef DREAM_UDTNAKPACKET_H
#define DREAM_UDTNAKPACKET_H

/**
 * udt未确认包
 */
#include "UDTControlPacket.h"

namespace Dream {

    class UDTNakPacket : public UDTControlPacket {

    public:
        UDTNakPacket(int32_t* lossSeqNumber, std::size_t lossSize) :
                UDTControlPacket(NAK) {
            if (lossSeqNumber != nullptr && lossSize > 0) {
                _lossSeqNumber = new int32_t[lossSize];
                _lossSize = lossSize;
                memcpy(_lossSeqNumber, lossSeqNumber, _lossSize);
            }
        }

        virtual ~UDTNakPacket() {
            if (_lossSeqNumber != nullptr) {
                delete []_lossSeqNumber;
                _lossSeqNumber = nullptr;
            }
        }

        virtual int encode(char *pBuf, std::size_t len) override {
            if (len < totalLength()) {
                return -1;
            }

            int encodePos = UDTControlPacket::encode(pBuf, len);

            if (encodePos >= 0) {
                for (int i = 0; i < _lossSize; ++i) {
                    ENCODE_INT32(pBuf + encodePos, *(_lossSeqNumber + i), encodePos);
                }
            }

            return encodePos;
        }

        virtual int decode(const char *pBuf, std::size_t len) override {
            if (len < headerLength()) {
                return -1;
            }

            int decodePos = UDTControlPacket::decode(pBuf, len);

            if (decodePos >= 0) {
                std::size_t leaveLen = len - decodePos;
                if (leaveLen > sizeof(uint32_t)) {
                    _lossSize = leaveLen / sizeof(uint32_t);
                    if (_lossSize > 0) {
                        _lossSeqNumber = new int32_t[_lossSize];
                        for (int i = 0; i < _lossSize; ++i) {
                            DECODE_INT32(pBuf + decodePos, *(_lossSeqNumber + i), decodePos);
                        }
                    }
                }
            }

            return decodePos;
        }

        static std::size_t headerLength() {
            return UDTControlPacket::headerLength();
        }

        virtual std::size_t totalLength() override {
            return headerLength() + sizeof(int32_t) * _lossSize;
        }

    private:
        int32_t *_lossSeqNumber;  // 丢失的序列号列表
        std::size_t _lossSize;  // 丢失数量
    };

}

#endif //VMSDKDEMO_ANDROID_UDTSYNPACKET_H
