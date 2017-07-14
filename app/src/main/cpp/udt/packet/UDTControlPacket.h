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
        static unsigned short RESERVED_MAX = UINT16_MAX;
        static unsigned ADDITIONAL_INFO_MAX = UINT32_MAX;

        enum ControlPacketType {
            SYN = 0x0,  // 连接握手
            KEEP_ALIVE = 0x1,  // 保活
            ACK = 0x2,  // 确认
            NAK = 0x3, // 未确认
            UNUSED = 0x4,  // 未使用
            SHUTDOWN = 0x5,  // 关闭
            ACK2 = 0x6,  // 确认包的确认包
            MDR = 0x7,  // Message Drop Request， 消息丢失请求，消息存在丢失列表中超出了生命周期，那么就放弃发送并发送mdq
            USER = 0x7fff  // 用户自定义控制包
        };

    private:
        const char *TAG = "UDTControlPacket";

    public:
        UDTControlPacket(ControlPacketType controlPacketType, unsigned short reserved = 0,
                         unsigned additionalInfo = 0)
                : UDTBasePacket(true), _controlPacketType(controlPacketType), _reserved(reserved),
                  _additionalInfo(additionalInfo) {
        }

        virtual ~UDTDataPacket() = default;

        virtual int encode(char *pBuf, std::size_t len) override {
            if (len < totalLength()) {
                return -1;
            }

            int encodePos = 0;

            unsigned short tmp = _controlPacketType;
            if (isControl()) {  // control
                tmp |= (1 << 15);
            }
            ENCODE_SHORT(pBuf, tmp, encodePos);  // type

            ENCODE_SHORT(pBuf + encodePos, _reserved, encodePos);  // reserved

            ENCODE_INT(pBuf + encodePos, _additionalInfo, encodePos);  // additional info

            encodeTimestamp(pBuf + encodePos, encodePos);
            encodeDstSocketId(pBuf + encodePos, encodePos);

            return encodePos;
        }

        virtual int decode(const char *pBuf, std::size_t len) override {
            if (len < headerLength()) {
                return -1;
            }

            // control
            decodeControl((const unsigned char) *pBuf);

            int decodePos = 0;
            unsigned short tmp;
            DECODE_SHORT(pBuf, tmp, decodePos);
            tmp &= 0x7fff;  // type

            _controlPacketType = ControlPacketType(tmp);

            DECODE_SHORT(pBuf + decodePos, _reserved, decodePos);  // reserved
            DECODE_INT(pBuf + decodePos, _additionalInfo, decodePos);  // additional info

            decodeTimestamp(pBuf + decodePos, decodePos);
            decodeDstSocketId(pBuf + decodePos, decodePos);

            return decodePos;
        }

        virtual std::size_t headerLength() override {
            return UDTBasePacket::headerLength() + sizeof(short) + sizeof(_reserved) +
                   sizeof(_additionalInfo);
        }

        virtual std::size_t totalLength() override {
            return headerLength();
        }

        void setAdditionalInfo(unsigned additionalInfo) {
            _additionalInfo = additionalInfo;
        }

        unsigned getAdditionalInfo() {
            return _additionalInfo;
        }

    private:
        ControlPacketType _controlPacketType;  // 控制包类型
        unsigned short _reserved;  // 预留
        unsigned _additionalInfo;  // 附加信息
    };

}

#endif //VMSDKDEMO_ANDROID_UDTDATAPACKET_H
