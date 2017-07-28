//
// Created by zhou rui on 2017/7/11.
//

#ifndef DREAM_UDTCONTROLPACKET_H
#define DREAM_UDTCONTROLPACKET_H

#include "UDTBasePacket.h"

/**
 * udt控制包
 */
namespace Dream {

    class UDTControlPacket : public UDTBasePacket {

    public:
        static const uint16_t RESERVED_MAX = UINT16_MAX;
        static const int32_t ADDITIONAL_INFO_MAX = INT32_MAX;

        enum ControlPacketType {
            SYN = 0x0,  // 连接握手
            KEEP_ALIVE = 0x1,  // 保活
            ACK = 0x2,  // 确认
            NAK = 0x3, // 未确认
            UNUSED = 0x4,  // 未使用
            SHUTDOWN = 0x5,  // 关闭
            ACK2 = 0x6,  // 确认包的确认包
            MDR = 0x7,  // Message Drop Request， 消息丢失请求，消息存在丢失列表中超出了生命周期，那么就放弃发送并发送mdq
            USER = 0x7fff,  // 用户自定义控制包
            UNKNOW = 0xffff  // 未知时
        };

    private:
        const char *TAG = "UDTControlPacket";

    public:
        UDTControlPacket(ControlPacketType controlPacketType, uint16_t reserved = 0,
                         int32_t additionalInfo = 0)
                : UDTBasePacket(true), _controlPacketType(controlPacketType), _reserved(reserved),
                  _additionalInfo(additionalInfo) {
        }

        virtual ~UDTControlPacket() = default;

        virtual int encode(char *pBuf, std::size_t len) override {
            if (len < totalLength()) {
                return -1;
            }

            int encodePos = 0;

            unsigned short tmp = _controlPacketType;
            if (isControl()) {  // control
                tmp |= (1 << 15);
            }
            ENCODE_INT16(pBuf, tmp, encodePos);  // type

            ENCODE_INT16(pBuf + encodePos, _reserved, encodePos);  // reserved

            ENCODE_INT32(pBuf + encodePos, _additionalInfo, encodePos);  // additional info

            encodeTimestamp(pBuf + encodePos, encodePos);
            encodeDstSocketId(pBuf + encodePos, encodePos);

            return encodePos;
        }

        static bool decodeControlTypeStatic(const char *pBuf, std::size_t len, ControlPacketType& controlPacketType) {
            if (len < headerLength()) {
                return false;
            }
            if (!UDTBasePacket::decodeControlStatic((const unsigned char) *pBuf)) {
                return false;
            }
            int decodePos = 0;
            uint16_t tmp;
            DECODE_INT16(pBuf, tmp, decodePos);
            tmp &= 0x7fff;  // type
            controlPacketType = ControlPacketType(tmp);
            return true;
        }

        virtual int decode(const char *pBuf, std::size_t len) override {
            if (len < headerLength()) {
                return -1;
            }

            // control
            decodeControl((const unsigned char) *pBuf);

            int decodePos = 0;
            uint16_t tmp;
            DECODE_INT16(pBuf, tmp, decodePos);
            tmp &= 0x7fff;  // type

            _controlPacketType = ControlPacketType(tmp);

            DECODE_INT16(pBuf + decodePos, _reserved, decodePos);  // reserved
            DECODE_INT32(pBuf + decodePos, _additionalInfo, decodePos);  // additional info

            decodeTimestamp(pBuf + decodePos, decodePos);
            decodeDstSocketId(pBuf + decodePos, decodePos);

            return decodePos;
        }

        static std::size_t headerLength() {
            return UDTBasePacket::headerLength() + sizeof(uint16_t) + sizeof(uint16_t) +
                   sizeof(int32_t);
        }

        virtual std::size_t totalLength() override {
            return headerLength();
        }

        void setAdditionalInfo(int32_t additionalInfo) {
            _additionalInfo = additionalInfo;
        }

        int32_t getAdditionalInfo() {
            return _additionalInfo;
        }

    private:
        ControlPacketType _controlPacketType;  // 控制包类型
        uint16_t _reserved;  // 预留
        int32_t _additionalInfo;  // 附加信息
    };

}

#endif //VMSDKDEMO_ANDROID_UDTDATAPACKET_H
