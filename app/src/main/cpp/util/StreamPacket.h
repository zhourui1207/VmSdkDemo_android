/*
 * StreamPacket.h
 *
 *  Created on: 2016年9月27日
 *      Author: zhourui
 *      码流包
 */


#ifndef STREAMPACKET_H_
#define STREAMPACKET_H_

#include "BasePacket.h"

namespace Dream {

    class StreamPacket : public BasePacket {
    public:
        static const unsigned HEADER_LENGTH = 2 * sizeof(int);
        static const unsigned BODY_MAX_SIZE = 65515;

        StreamPacket()
                : _length(HEADER_LENGTH), _msgType(0), _dataLen(0) {
        }

        StreamPacket(unsigned msgType)
                : _length(HEADER_LENGTH), _msgType(msgType), _dataLen(0) {

        }

        virtual ~StreamPacket() {
//    if (_data != nullptr) {
//      delete[] _data;
//    }
        }

        // 设置数据
        void setData(const char *pBuf, std::size_t len) {
            if (len > BODY_MAX_SIZE) {
                throw std::exception();
            }
            if (len > 0) {
                _dataLen = len;
//      _data = new char[_dataLen];
                memcpy(_data, pBuf, _dataLen);
            }
        }

        // ---------------------派生类重写开始---------------------
        virtual std::size_t computeLength() const override {
            return HEADER_LENGTH + _dataLen;
        }

        virtual int encode(char *pBuf, std::size_t len) override {
            if (len < HEADER_LENGTH) {
                return -1;
            }

            int pos = 0;
            ENCODE_INT(pBuf + pos, _length, pos);
            ENCODE_INT(pBuf + pos, _msgType, pos);
            if (_dataLen <= len - HEADER_LENGTH) {
                memcpy(pBuf + pos, _data, _dataLen);
            } else {
                return -1;
            }
            return computeLength();
        }

        virtual int decode(char *pBuf, std::size_t len) override {
            if (len < HEADER_LENGTH) {
                return -1;
            }

            int pos = 0;
            DECODE_INT(pBuf + pos, _length, pos);
            DECODE_INT(pBuf + pos, _msgType, pos);
            int dataLen = _length - HEADER_LENGTH;
            if (dataLen > 0 && dataLen + HEADER_LENGTH <= len) {
//      _data = new char[dataLen];
                _dataLen = dataLen;
                memcpy(_data, pBuf + pos, dataLen);
            } else {
                return -1;
            }
            return computeLength();
        }
        // ---------------------派生类重写结束---------------------

        void length(std::size_t len) override {
            _length = len;
        }

        unsigned length() const {
            return _length;
        }

        unsigned msgType() const {
            return _msgType;
        }

        void msgType(unsigned msgType) {
            _msgType = msgType;
        }

        char *data() const {
            return const_cast<char *>(_data);
        }

        std::size_t dataLen() const {
            return _dataLen;
        }

    private:
        /**
         * 消息结构定义：包总长＋消息类型＋数据
         */
        unsigned _length;  // 包总长
        unsigned _msgType;  // 消息类型
        std::size_t _dataLen;  // 数据长度
        char _data[BODY_MAX_SIZE];  // 数据
    };

} /* namespace Dream */

#endif /* MESSAGEPACKET_H_ */
