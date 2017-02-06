/*
 * BasePacket.h
 *
 *  Created on: 2016年9月28日
 *      Author: zhourui
 *      写包时继承这个父类，通信模版调用的函数都是使用这个类中的函数，至少需要知道分包的方法
 */

#ifndef BASEPACKET_H_
#define BASEPACKET_H_

#include <cstring>

#include "public/platform.h"

namespace Dream {

#define ENCODE_INT(pBuf, nValue, nPos) do \
    {\
      unsigned uTmp = htonl(nValue);\
      memcpy(pBuf, &uTmp, sizeof(int));\
      nPos += sizeof(int);\
    } while (0);

#define DECODE_INT(pBuf, nValue, nPos) do \
    {\
      memcpy(&nValue, pBuf, sizeof(int));\
      nValue = ntohl(nValue);\
      nPos += sizeof(int);\
    } while (0);

#define ENCODE_STRING(pBuf, sValue, nPos) do \
    {\
      memcpy(pBuf, sValue.c_str(), std::size_t(sValue.length() + 1)); \
      nPos += (sValue.length() + 1);\
    } while(0);

#define DECODE_STRING(pBuf, sValue, nPos) do \
    {\
      sValue = pBuf; \
      nPos += (sValue.length() + 1);\
    } while(0);


    class BasePacket {

    public:
        BasePacket() = default;

        virtual ~BasePacket() = default;

        // 计算包长度
        virtual std::size_t computeLength() const = 0;

        // 设置包长
        virtual void length(std::size_t len) = 0;

        // 将包内容写入buff，返回写入长度
        virtual int encode(char *pBuf, std::size_t len) = 0;

        // 将buff内容解析成包，返回已解析的长度
        virtual int decode(char *pBuf, std::size_t len) = 0;
    };

} /* namespace Dream */

#endif /* BASEPACKET_H_ */
