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
        UDTSynPacket(uint32_t version = 0, uint32_t socketType = 0, int32_t initSeqNumber = 0,
                     uint32_t maxPacketSize = 0, uint32_t maxWindowSize = 0,
                     int32_t connectionType = 0, uint32_t socketId = 0, uint32_t synCookie = 0,
                     uint32_t *remoteAddr = nullptr/*长度必须是4*/) :
                UDTControlPacket(SYN), _version(version), _socketType(socketType),
                _initSeqNumber(initSeqNumber), _maxPacketSize(maxPacketSize),
                _maxWindowSize(maxWindowSize), _connectionType(connectionType),
                _socketId(socketId), _synCookie(synCookie) {
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

            ENCODE_INT32(pBuf + encodePos, _version, encodePos);
            ENCODE_INT32(pBuf + encodePos, _socketType, encodePos);
            ENCODE_INT32(pBuf + encodePos, _initSeqNumber, encodePos);
            ENCODE_INT32(pBuf + encodePos, _maxPacketSize, encodePos);
            ENCODE_INT32(pBuf + encodePos, _maxWindowSize, encodePos);
            ENCODE_INT32(pBuf + encodePos, _connectionType, encodePos);
            ENCODE_INT32(pBuf + encodePos, _socketId, encodePos);
            ENCODE_INT32(pBuf + encodePos, _synCookie, encodePos);
            for (int i = 0; i < 4; ++i) {
                ENCODE_INT32(pBuf + encodePos, *(_address + i), encodePos);
            }

            return encodePos;
        }

        virtual int decode(const char *pBuf, std::size_t len) override {
            if (len < headerLength()) {
                return -1;
            }

            int decodePos = UDTControlPacket::decode(pBuf, len);

            DECODE_INT32(pBuf + decodePos, _version, decodePos);
            DECODE_INT32(pBuf + decodePos, _socketType, decodePos);
            DECODE_INT32(pBuf + decodePos, _initSeqNumber, decodePos);
            DECODE_INT32(pBuf + decodePos, _maxPacketSize, decodePos);
            DECODE_INT32(pBuf + decodePos, _maxWindowSize, decodePos);
            DECODE_INT32(pBuf + decodePos, _connectionType, decodePos);
            DECODE_INT32(pBuf + decodePos, _socketId, decodePos);
            DECODE_INT32(pBuf + decodePos, _synCookie, decodePos);
            for (int i = 0; i < 4; ++i) {
                DECODE_INT32(pBuf + decodePos, *(_address + i), decodePos);
            }

            return decodePos;
        }

        static std::size_t headerLength() {
            return UDTControlPacket::headerLength() + sizeof(uint32_t) + sizeof(uint32_t) +
                   sizeof(int32_t) + sizeof(uint32_t) + sizeof(uint32_t) +
                   sizeof(int32_t) + sizeof(uint32_t) + sizeof(uint32_t) +
                   sizeof(uint32_t) * 4;
        }

        virtual std::size_t totalLength() override {
            return headerLength();
        }

        int32_t connectionType() const {
            return _connectionType;
        }

    private:
        uint32_t _version;
        uint32_t _socketType;  // stream or dgram 流/报文
        int32_t _initSeqNumber;
        uint32_t _maxPacketSize;
        uint32_t _maxWindowSize;
        int32_t _connectionType;  // regular or rendezvous  正常sc/同时连接p2p
        uint32_t _socketId;
        uint32_t _synCookie;
        uint32_t _address[4];  // 兼容ipv6，采用128位，如果是ipv4，使用最后32位即可

    };

}

#endif //VMSDKDEMO_ANDROID_UDTSYNPACKET_H
