//
// Created by zhou rui on 2017/7/12.
//

#ifndef DREAM_UDTSYNPACKET_H
#define DREAM_UDTSYNPACKET_H

/**
 * udt握手包
 */
#include "UDTControlPacket.h"

namespace Dream {

    class UDTSynPacket : public UDTControlPacket {

    public:
        UDTSynPacket(unsigned version, unsigned socketType, unsigned initSeqNumber,
                     unsigned maxPacketSize, unsigned maxWindowSize, unsigned connectionType,
                     unsigned socketId, unsigned synCookie, unsigned* remoteAddr/*长度必须是4*/) :
                UDTControlPacket(SYN), _version(version), _socketType(socketType),
                _initSeqNumber(initSeqNumber), _maxPacketSize(maxPacketSize),
                _maxWindowSize(maxWindowSize), _connectionType(connectionType),
                _socketId(_socketId), _synCookie(synCookie) {
            memset(_address, 0, 4);
            if (remoteAddr != nullptr) {
                memcpy(_address, remoteAddr, 4);
            }
        }

        virtual ~UDTSynPacket() = default;

        virtual int encode(char *pBuf, std::size_t len) override {
            if (len < totalLength()) {
                return -1;
            }

            int encodePos = UDTControlPacket::encode(pBuf, len);

            ENCODE_INT(pBuf + encodePos, _version, encodePos);
            ENCODE_INT(pBuf + encodePos, _socketType, encodePos);
            ENCODE_INT(pBuf + encodePos, _initSeqNumber, encodePos);
            ENCODE_INT(pBuf + encodePos, _maxPacketSize, encodePos);
            ENCODE_INT(pBuf + encodePos, _maxWindowSize, encodePos);
            ENCODE_INT(pBuf + encodePos, _connectionType, encodePos);
            ENCODE_INT(pBuf + encodePos, _socketId, encodePos);
            ENCODE_INT(pBuf + encodePos, _synCookie, encodePos);
            for (int i = 0; i < 4; ++i) {
                ENCODE_INT(pBuf + encodePos, *(_address + i), encodePos);
            }

            return encodePos;
        }

        virtual int decode(const char *pBuf, std::size_t len) override {
            if (len < headerLength()) {
                return -1;
            }

            int decodePos = UDTControlPacket::decode(pBuf, len);

            DECODE_INT(pBuf + decodePos, _version, decodePos);
            DECODE_INT(pBuf + decodePos, _socketType, decodePos);
            DECODE_INT(pBuf + decodePos, _initSeqNumber, decodePos);
            DECODE_INT(pBuf + decodePos, _maxPacketSize, decodePos);
            DECODE_INT(pBuf + decodePos, _maxWindowSize, decodePos);
            DECODE_INT(pBuf + decodePos, _connectionType, decodePos);
            DECODE_INT(pBuf + decodePos, _socketId, decodePos);
            DECODE_INT(pBuf + decodePos, _synCookie, decodePos);
            for (int i = 0; i < 4; ++i) {
                DECODE_INT(pBuf + decodePos, *(_address + i), decodePos);
            }

            return decodePos;
        }

        virtual std::size_t headerLength() override {
            return UDTControlPacket::headerLength() + sizeof(_version) + sizeof(_socketType) +
                   sizeof(_initSeqNumber) + sizeof(_maxPacketSize) + sizeof(_maxWindowSize) +
                   sizeof(_connectionType) + sizeof(_socketId) + sizeof(_synCookie) +
                   sizeof(unsigned) * 4;
        }

        virtual std::size_t totalLength() override {
            return headerLength();
        }

    private:
        unsigned _version;
        unsigned _socketType;  // stream or dgram 流/报文
        unsigned _initSeqNumber;
        unsigned _maxPacketSize;
        unsigned _maxWindowSize;
        unsigned _connectionType;  // regular or rendezvous  正常sc/同时连接p2p
        unsigned _socketId;
        unsigned _synCookie;
        unsigned _address[4];  // 兼容ipv6，采用128位，如果是ipv4，使用最后32位即可

    };

}

#endif //VMSDKDEMO_ANDROID_UDTSYNPACKET_H
