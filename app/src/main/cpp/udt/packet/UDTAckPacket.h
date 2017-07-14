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
        UDTAckPacket(unsigned ackSeqNumber, unsigned seqNumber, unsigned rttUs,
                     unsigned rttVariance, unsigned bufferSize, unsigned receiveRate,
                     unsigned linkCapacity) :
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

            ENCODE_INT(pBuf + encodePos, _seqNumber, encodePos);
            ENCODE_INT(pBuf + encodePos, _rttUs, encodePos);
            ENCODE_INT(pBuf + encodePos, _rttVariance, encodePos);
            ENCODE_INT(pBuf + encodePos, _availableBufSize, encodePos);
            ENCODE_INT(pBuf + encodePos, _receiveRate, encodePos);
            ENCODE_INT(pBuf + encodePos, _linkCapacity, encodePos);

            return encodePos;
        }

        virtual int decode(const char *pBuf, std::size_t len) override {
            if (len < headerLength()) {
                return -1;
            }

            int decodePos = UDTControlPacket::decode(pBuf, len);

            DECODE_INT(pBuf + decodePos, _seqNumber, decodePos);
            DECODE_INT(pBuf + decodePos, _rttUs, decodePos);
            DECODE_INT(pBuf + decodePos, _rttVariance, decodePos);
            DECODE_INT(pBuf + decodePos, _availableBufSize, decodePos);
            DECODE_INT(pBuf + decodePos, _receiveRate, decodePos);
            DECODE_INT(pBuf + decodePos, _linkCapacity, decodePos);

            return decodePos;
        }

        virtual std::size_t headerLength() override {
            return UDTControlPacket::headerLength() + sizeof(_seqNumber) + sizeof(_rttUs) +
                   sizeof(_rttVariance) + sizeof(_availableBufSize) + sizeof(_receiveRate) +
                   sizeof(_linkCapacity);
        }

        virtual std::size_t totalLength() override {
            return headerLength();
        }

    private:
        unsigned _seqNumber;  //  The packet sequence number to which all the previous packets have been received
        unsigned _rttUs;  // RTT microseconds
        unsigned _rttVariance;
        unsigned _availableBufSize;  // bytes
        unsigned _receiveRate;  // Packets receiving rate, in number of packets per second
        unsigned _linkCapacity;  // Estimated link capacity, in number of packets per second
    };

}

#endif //VMSDKDEMO_ANDROID_UDTSYNPACKET_H
