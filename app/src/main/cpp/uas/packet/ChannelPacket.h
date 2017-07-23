/*
 * ChannelPacket.h
 *
 *  Created on: 2016年10月2日
 *      Author: zhourui
 */

#ifndef CHANNELPACKET_H_
#define CHANNELPACKET_H_

#include "../../util/MsgPacket.h"
#include "PacketMsgType.h"

namespace Dream {

    class ChannelItem {
    public:
        ChannelItem() :
                _depId(0), _channelId(0), _channelType(0), _isOnLine(0), _videoState(0),
                _channelState(
                        0), _recordState(0) {
        }

        virtual ~ChannelItem() = default;

        std::size_t computeLength() const {
            return 7 * sizeof(int) + _fdId.length() + 1 + _channelName.length() + 1;
        }

    public:
        int _depId;
        std::string _fdId;
        int _channelId;
        unsigned _channelType;
        std::string _channelName;
        unsigned _isOnLine;
        unsigned _videoState;
        unsigned _channelState;
        unsigned _recordState;
    };

    using ChannelList = std::vector<ChannelItem>;

// 获取通道请求包
    class GetChannelReqPacket : public MsgPacket {
    public:
        enum TREE_TYPE {
            TREE_TYPE_DEP = 0,  // 行政树
            TREE_TYPE_PUB = 1  // 公共树
        };

    public:
        GetChannelReqPacket() :
                MsgPacket(MSG_GET_DEP_CHANNEL_REQ), _depId(0), _treeType(TREE_TYPE_DEP) {

        }

        virtual ~GetChannelReqPacket() = default;

        // ---------------------派生类重写开始---------------------
        virtual std::size_t computeLength() const override {
            return MsgPacket::computeLength() + 2 * sizeof(int);
        }

        virtual int encode(char *pBuf, std::size_t len) override {
            std::size_t dataLength = computeLength();
            if (len < dataLength) {
                return -1;
            }
            int usedLen = MsgPacket::encode(pBuf, len);
            if (usedLen < 0) {
                return -1;
            }

            char *pBody = pBuf + usedLen;

            int pos = 0;
            ENCODE_INT32(pBody + pos, _depId, pos);
            ENCODE_INT32(pBody + pos, _treeType, pos);

            return dataLength;
        }

        virtual int decode(char *pBuf, std::size_t len) override {
            int usedLen = MsgPacket::decode(pBuf, len);
            if (usedLen < 0) {
                return -1;
            }

            char *pBody = pBuf + usedLen;

            int pos = 0;
            DECODE_INT32(pBody + pos, _depId, pos);
            DECODE_INT32(pBody + pos, _treeType, pos);

            // 包长和实际长度不符
            if (computeLength() != length()) {
                return -1;
            }
            return length();
        }
        // ---------------------派生类重写结束---------------------

    public:
        int _depId;
        unsigned _treeType;
    };

// 获取通道响应包
    class GetChannelRespPacket : public MsgPacket {
    public:
        GetChannelRespPacket() :
                MsgPacket(MSG_GET_DEP_CHANNEL_RESP), _channelCounts(0) {

        }

        virtual ~GetChannelRespPacket() = default;

        // ---------------------派生类重写开始---------------------
        virtual std::size_t computeLength() const override {
            std::size_t len = 0;
            for (std::size_t i = 0; i < _channels.size(); ++i) {
                len += _channels[i].computeLength();
            }
            return MsgPacket::computeLength() + sizeof(int) + len;
        }

        virtual int encode(char *pBuf, std::size_t len) override {
            std::size_t dataLength = computeLength();
            if (len < dataLength) {
                return -1;
            }
            int usedLen = MsgPacket::encode(pBuf, len);
            if (usedLen < 0) {
                return -1;
            }

            char *pBody = pBuf + usedLen;

            int pos = 0;
            _channelCounts = _channels.size();
            ENCODE_INT32(pBody + pos, _channelCounts, pos);
            for (unsigned i = 0; i < _channelCounts; ++i) {
                ENCODE_INT32(pBody + pos, _channels[i]._depId, pos);
                ENCODE_STRING(pBody + pos, _channels[i]._fdId, pos);
                ENCODE_INT32(pBody + pos, _channels[i]._channelId, pos);
                ENCODE_INT32(pBody + pos, _channels[i]._channelType, pos);
                ENCODE_STRING(pBody + pos, _channels[i]._channelName, pos);
                ENCODE_INT32(pBody + pos, _channels[i]._isOnLine, pos);
                ENCODE_INT32(pBody + pos, _channels[i]._videoState, pos);
                ENCODE_INT32(pBody + pos, _channels[i]._channelState, pos);
                ENCODE_INT32(pBody + pos, _channels[i]._recordState, pos);
            }

            return dataLength;
        }

        virtual int decode(char *pBuf, std::size_t len) override {
            int usedLen = MsgPacket::decode(pBuf, len);
            if (usedLen < 0) {
                return -1;
            }

            char *pBody = pBuf + usedLen;

            int pos = 0;
            DECODE_INT32(pBody + pos, _channelCounts, pos);
            for (unsigned i = 0; i < _channelCounts; ++i) {
                ChannelItem item;
                DECODE_INT32(pBody + pos, item._depId, pos);
                DECODE_STRING(pBody + pos, item._fdId, pos);
                DECODE_INT32(pBody + pos, item._channelId, pos);
                DECODE_INT32(pBody + pos, item._channelType, pos);
                DECODE_STRING(pBody + pos, item._channelName, pos);
                DECODE_INT32(pBody + pos, item._isOnLine, pos);
                DECODE_INT32(pBody + pos, item._videoState, pos);
                DECODE_INT32(pBody + pos, item._channelState, pos);
                DECODE_INT32(pBody + pos, item._recordState, pos);
                _channels.push_back(item);
            }

            // 包长和实际长度不符
            if (computeLength() != length()) {
                return -1;
            }
            return length();
        }
        // ---------------------派生类重写结束---------------------

    public:
        unsigned _channelCounts;  // 数量
        ChannelList _channels;  // 通道列表
    };

} /* namespace Dream */

#endif /* DEPTREEPACKET_H_ */
