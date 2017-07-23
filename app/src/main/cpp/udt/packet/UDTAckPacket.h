//
// Created by zhou rui on 2017/7/12.
//

#ifndef DREAM_UDTACKPACKET_H
#define DREAM_UDTACKPACKET_H

/**
 * udt确认包
 */
#include "UDTControlPacket.h"

namespace Dream {

    class UDTAckPacket : public UDTControlPacket {

    public:
        UDTAckPacket(int32_t ackSeqNumber, int32_t seqNumber, uint32_t rttUs,
                     uint32_t rttVariance, uint32_t bufferSize, uint32_t receiveRate,
                     uint32_t linkCapacity) :
                UDTControlPacket(ACK, 0, ackSeqNumber), _seqNumber(seqNumber), _rttUs(rttUs),
                _rttVariance(rttVariance), _availableBufSize(bufferSize), _receiveRate(receiveRate),
                _linkCapacity(linkCapacity) {

        }

        virtual ~UDTAckPacket() = default;

        virtual int encode(char *pBuf, std::size_t len) override {
            if (len < totalLength()) {
                return -1;
            }

            int encodePos = UDTControlPacket::encode(pBuf, len);

            ENCODE_INT32(pBuf + encodePos, _seqNumber, encodePos);
            ENCODE_INT32(pBuf + encodePos, _rttUs, encodePos);
            ENCODE_INT32(pBuf + encodePos, _rttVariance, encodePos);
            ENCODE_INT32(pBuf + encodePos, _availableBufSize, encodePos);
            ENCODE_INT32(pBuf + encodePos, _receiveRate, encodePos);
            ENCODE_INT32(pBuf + encodePos, _linkCapacity, encodePos);

            return encodePos;
        }

        virtual int decode(const char *pBuf, std::size_t len) override {
            if (len < headerLength()) {
                return -1;
            }

            int decodePos = UDTControlPacket::decode(pBuf, len);

            DECODE_INT32(pBuf + decodePos, _seqNumber, decodePos);
            DECODE_INT32(pBuf + decodePos, _rttUs, decodePos);
            DECODE_INT32(pBuf + decodePos, _rttVariance, decodePos);
            DECODE_INT32(pBuf + decodePos, _availableBufSize, decodePos);
            DECODE_INT32(pBuf + decodePos, _receiveRate, decodePos);
            DECODE_INT32(pBuf + decodePos, _linkCapacity, decodePos);

            return decodePos;
        }

        static std::size_t headerLength() {
            return UDTControlPacket::headerLength() + sizeof(int32_t) + sizeof(uint32_t) +
                   sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) +
                   sizeof(uint32_t);
        }

        virtual std::size_t totalLength() override {
            return headerLength();
        }

    private:
        int32_t _seqNumber;  //  The packet sequence number to which all the previous packets have been received
        uint32_t _rttUs;  // RTT microseconds
        uint32_t _rttVariance;
        uint32_t _availableBufSize;  // bytes
        uint32_t _receiveRate;  // Packets receiving rate, in number of packets per second
        uint32_t _linkCapacity;  // Estimated link capacity, in number of packets per second
    };

}

#endif //VMSDKDEMO_ANDROID_UDTSYNPACKET_H
