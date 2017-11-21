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
        static const uint32_t MSG_NUMBER_MAX = 536870911;  // 2的29次方-1

        enum DataPacketType {
            MIDDLE = 0,  // 中间
            STOP = 1,  // 结束
            START = 2,  // 起始
            ONLY = 3 // 单独
        };

    private:
        const char *TAG = "UDTDataPacket";

    public:
        UDTDataPacket(const char *pBuf = nullptr, std::size_t len = 0, int32_t seqNumber = -1,
                      DataPacketType dataPacketType = ONLY, bool order = true,
                      uint32_t msgNumber = 0)
                : UDTBasePacket(false), _dataBuf(nullptr), _dataLen(0), _seqNumber(seqNumber),
                  _dataPacketType(dataPacketType), _order(order), _msgNumber(msgNumber),
                  _usSendTimestamp(0), _drop(false) {
            if (pBuf != nullptr && len > 0) {
                _dataBuf = new char[len];
                _dataLen = len;
                memcpy(_dataBuf, pBuf, _dataLen);
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

            int32_t tmp = _seqNumber;  // sequence number
            if (isControl()) {  // control
                tmp |= (1 << 31);
            }
            ENCODE_INT32(pBuf, tmp, encodePos);

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

            ENCODE_INT32(pBuf + encodePos, tmp, encodePos);

            encodeTimestamp(pBuf + encodePos, encodePos);  // timestamp
            encodeDstSocketId(pBuf + encodePos, encodePos);  // dst socket id

            // data
            if (_dataBuf != nullptr && _dataLen > 0) {
                memcpy(pBuf + encodePos, _dataBuf, _dataLen);
            }

            return encodePos + _dataLen;
        }

        virtual int decode(const char *pBuf, std::size_t len) override {
            if (len < headerLength()) {
                return -1;
            }

            // control
            decodeControl((const unsigned char) *pBuf);

            int decodePos = 0;
            uint32_t tmp;
            DECODE_INT32(pBuf, tmp, decodePos);
            _seqNumber = (tmp & 0x7fffffff);  // sequence number

            int ff = ((*(pBuf + decodePos) & 0xc0) >> 6);
            _dataPacketType = DataPacketType(ff);  // data packet type

            _order = (*(pBuf + decodePos) & 0x20) != 0;  // order

            DECODE_INT32(pBuf + decodePos, tmp, decodePos);
            _msgNumber = (tmp & 0x1fffffff);  // msg number

            decodeTimestamp(pBuf + decodePos, decodePos);
            decodeDstSocketId(pBuf + decodePos, decodePos);

            int dataLen = len - decodePos;
            if (dataLen > 0) {
                _dataBuf = new char[dataLen];
                _dataLen = (size_t) dataLen;
                memcpy(_dataBuf, pBuf + decodePos, _dataLen);
            }

            return len;
        }

        static std::size_t headerLength() {
            return UDTBasePacket::headerLength() + sizeof(int32_t) + sizeof(uint32_t);
        }

        virtual std::size_t totalLength() override {
            return headerLength() + _dataLen;
        }

        int32_t seqNumber() const {
            return _seqNumber;
        }
        
        void setSeqNumber(int32_t seqNumber) {
            _seqNumber = seqNumber;
        }
        
        uint32_t msgNumber() const {
            return _msgNumber;
        }

        uint64_t sendTimestamp() const {
            return _usSendTimestamp;
        }

        void setSendTimestamp(uint64_t timestamp) {
            _usSendTimestamp = timestamp;
        }

        void setDrop(bool drop) {
            _drop = drop;
        }

        bool isDrop() const {
            return _drop;
        }

        const char* data() const {
            return _dataBuf;
        }

        std::size_t len() const {
            return _dataLen;
        }
        
        DataPacketType dataPacketType() const {
            return _dataPacketType;
        }

    private:
        char *_dataBuf;  // 数据
        std::size_t _dataLen;  // 数据长度
        int32_t _seqNumber;  // 序列号
        DataPacketType _dataPacketType;  // 数据包类型
        bool _order;  // 是否按顺序
        uint32_t _msgNumber;  // 消息序列号

        // 加上一个高精度的时间戳记录发送或接收时间, base包里默认的时间戳是32位，无法表示微秒
        uint64_t _usSendTimestamp;
        bool _drop;  // 是否丢弃
    };

}

#endif //VMSDKDEMO_ANDROID_UDTDATAPACKET_H
