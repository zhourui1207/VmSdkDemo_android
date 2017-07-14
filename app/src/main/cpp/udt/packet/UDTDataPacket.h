//
// Created by zhou rui on 2017/7/11.
//

#ifndef DREAM_UDTDATAPACKET_H
#define DREAM_UDTDATAPACKET_H

#include "UDTBasePacket.h"

/**
 * udt数据包
 */

namespace Dream {

    class UDTDataPacket : public UDTBasePacket {

    public:
        static unsigned MSG_NUMBER_MAX = 536870911;  // 2的29次方-1

        enum DataPacketType {
            MIDDLE = 0,  // 中间
            STOP = 1,  // 结束
            START = 2,  // 起始
            ONLY = 3 // 单独
        };

    private:
        const char *TAG = "UDTDataPacket";

    public:
        UDTDataPacket(const char *pBuf, std::size_t len, unsigned seqNumber,
                      DataPacketType dataPacketType, bool order, unsigned msgNumber = 0)
                : UDTBasePacket(false), _dataBuf(nullptr), _dataLen(0), _seqNumber(seqNumber),
                  _dataPacketType(dataPacketType), _order(order), _msgNumber(msgNumber) {
            if (pBuf != nullptr && len > 0) {
                _dataBuf = new char[len];
                _dataLen = len;
            }
        }

        virtual ~UDTDataPacket() {
            if (_dataBuf != nullptr) {
                delete[]_dataBuf;
                _dataBuf = nullptr;
            }
        }

        virtual int encode(char *pBuf, std::size_t len) override {
            if (len < totalLength()) {
                return -1;
            }

            int encodePos = 0;

            unsigned tmp = _seqNumber;  // sequence number
            if (isControl()) {  // control
                tmp |= (1 << 31);
            }
            ENCODE_INT(pBuf, tmp, encodePos);

            tmp = _msgNumber;
            switch (_dataPacketType) {  // FF
                case MIDDLE:  // 00  0
                    break;
                case STOP:  // 01  1
                    tmp |= (1 << 30);
                    break;
                case START:  // 10  2
                    tmp |= (2 << 30);
                    break;
                case ONLY:  // 11  3
                    tmp |= (3 << 30);
                    break;
                default:
                    LOGE(TAG, "The packet type[%d] unsupported !\n", _dataPacketType);
                    tmp |= (3 << 30);
                    break;
            }
            if (_order) {  // O
                tmp |= (1 << 29);
            }

            ENCODE_INT(pBuf + encodePos, tmp, encodePos);

            encodeTimestamp(pBuf + encodePos, encodePos);  // timestamp
            encodeDstSocketId(pBuf + encodePos, encodePos);  // dst socket id

            // data
            if (_dataBuf != nullptr && _dataLen > 0) {
                memcpy(pBuf + encodePos, _dataBuf, _dataLen);
            }

            return encodePos;
        }

        virtual int decode(const char *pBuf, std::size_t len) override {
            if (len < headerLength()) {
                return -1;
            }

            // control
            decodeControl((const unsigned char) *pBuf);

            int decodePos = 0;
            unsigned tmp;
            DECODE_INT(pBuf, tmp, decodePos);
            _seqNumber = (tmp & 0x7fffffff);  // sequence number

            int ff = ((*(pBuf + decodePos) & 0xc0) >> 6);
            _dataPacketType = DataPacketType(ff);  // data packet type

            _order = (*(pBuf + decodePos) & 0x20) != 0;  // order

            DECODE_INT(pBuf + decodePos, tmp, decodePos);
            _msgNumber = (tmp & 0x1fffffff);  // msg number

            decodeTimestamp(pBuf + decodePos, decodePos);
            decodeDstSocketId(pBuf + decodePos, decodePos);

            return decodePos;
        }

        virtual std::size_t headerLength() override {
            return UDTBasePacket::headerLength() + sizeof(_seqNumber) + sizeof(_msgNumber);
        }

        virtual std::size_t totalLength() override {
            return headerLength() + _dataLen;
        }


    private:
        char *_dataBuf;  // 数据
        std::size_t _dataLen;  // 数据长度
        unsigned _seqNumber;  // 序列号
        DataPacketType _dataPacketType;  // 数据包类型
        bool _order;  // 是否按顺序
        unsigned _msgNumber;  // 消息序列号
    };

}

#endif //VMSDKDEMO_ANDROID_UDTDATAPACKET_H
