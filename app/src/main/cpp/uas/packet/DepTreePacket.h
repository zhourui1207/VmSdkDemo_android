/*
 * DepTreePacket.h
 *
 *  Created on: 2016年10月2日
 *      Author: zhourui
 */

#ifndef DEPTREEPACKET_H_
#define DEPTREEPACKET_H_

#include "../../util/MsgPacket.h"
#include "PacketMsgType.h"

namespace Dream {

class DepTreeItem {
public:
  DepTreeItem() : _depId(0), _parentId(0), _onlineChannelCounts(0), _offlineChannelCounts(0) {
  }

  virtual ~DepTreeItem() = default;

  std::size_t computeLength() const {
    return 4 * sizeof(int) + _depName.length() + 1;
  }

public:
  int _depId;
  std::string _depName;
  int _parentId;
  unsigned _onlineChannelCounts;
  unsigned _offlineChannelCounts;
};
using DepTreeList = std::vector<DepTreeItem>;

// 获取行政树请求包
class GetDepTreeReqPacket: public MsgPacket {
public:
  enum TREE_TYPE {
    TREE_TYPE_DEP = 0,  // 行政树
    TREE_TYPE_PUB = 1  // 公共树
  };

public:
  GetDepTreeReqPacket() :
      MsgPacket(MSG_GET_DEP_TREE_REQ), _treeType(0) {

  }
  virtual ~GetDepTreeReqPacket() = default;

  // ---------------------派生类重写开始---------------------
  virtual std::size_t computeLength() const override {
    return MsgPacket::computeLength() + sizeof(int);
  }

  virtual int encode(char* pBuf, std::size_t len) override {
    std::size_t dataLength = computeLength();
    if (len < dataLength) {
      return -1;
    }
    int usedLen = MsgPacket::encode(pBuf, len);
    if (usedLen < 0) {
      return -1;
    }

    char* pBody = pBuf + usedLen;

    int pos = 0;
    ENCODE_INT(pBody + pos, _treeType, pos);

    return dataLength;
  }

  virtual int decode(char* pBuf, std::size_t len) override {
    int usedLen = MsgPacket::decode(pBuf, len);
    if (usedLen < 0) {
      return -1;
    }

    char* pBody = pBuf + usedLen;

    int pos = 0;
    DECODE_INT(pBody + pos, _treeType, pos);

    // 包长和实际长度不符
    if (computeLength() != length()) {
      return -1;
    }
    return length();
  }
  // ---------------------派生类重写结束---------------------

public:
  unsigned _treeType;
};

// 获取行政树响应包
class GetDepTreeRespPacket: public MsgPacket {
public:
  GetDepTreeRespPacket() :
      MsgPacket(MSG_GET_DEP_TREE_RESP), _depCounts(0) {

  }
  virtual ~GetDepTreeRespPacket() = default;

  // ---------------------派生类重写开始---------------------
  virtual std::size_t computeLength() const override {
    std::size_t len = 0;
    for (std::size_t i = 0; i < _depTrees.size(); ++i) {
      len += _depTrees[i].computeLength();
    }
    return MsgPacket::computeLength() + sizeof(int) + len;
  }

  virtual int encode(char* pBuf, std::size_t len) override {
    std::size_t dataLength = computeLength();
    if (len < dataLength) {
      return -1;
    }
    int usedLen = MsgPacket::encode(pBuf, len);
    if (usedLen < 0) {
      return -1;
    }

    char* pBody = pBuf + usedLen;

    int pos = 0;
    _depCounts = _depTrees.size();
    ENCODE_INT(pBody + pos, _depCounts, pos);
    for (unsigned i = 0; i < _depCounts; ++i) {
      ENCODE_INT(pBody + pos, _depTrees[i]._depId, pos);
      ENCODE_STRING(pBody + pos, _depTrees[i]._depName, pos);
      ENCODE_INT(pBody + pos, _depTrees[i]._parentId, pos);
      ENCODE_INT(pBody + pos, _depTrees[i]._onlineChannelCounts, pos);
      ENCODE_INT(pBody + pos, _depTrees[i]._offlineChannelCounts, pos);
    }

    return dataLength;
  }

  virtual int decode(char* pBuf, std::size_t len) override {
    int usedLen = MsgPacket::decode(pBuf, len);
    if (usedLen < 0) {
      return -1;
    }

    char* pBody = pBuf + usedLen;

    int pos = 0;
    DECODE_INT(pBody + pos, _depCounts, pos);
    for (unsigned i = 0; i < _depCounts; ++i) {
      DepTreeItem item;
      DECODE_INT(pBody + pos, item._depId, pos);
      DECODE_STRING(pBody + pos, item._depName, pos);
      DECODE_INT(pBody + pos, item._parentId, pos);
      DECODE_INT(pBody + pos, item._onlineChannelCounts, pos);
      DECODE_INT(pBody + pos, item._offlineChannelCounts, pos);
      _depTrees.push_back(item);
    }

    // 包长和实际长度不符
    if (computeLength() != length()) {
      return -1;
    }
    return length();
  }
  // ---------------------派生类重写结束---------------------

public:
  unsigned _depCounts;  // 数量
  DepTreeList _depTrees;  // 树列表
};

} /* namespace Dream */

#endif /* DEPTREEPACKET_H_ */
