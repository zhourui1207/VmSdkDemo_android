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
        virtual int decode(const char *pBuf, std::size_t len) = 0;
    };

} /* namespace Dream */

#endif /* BASEPACKET_H_ */
