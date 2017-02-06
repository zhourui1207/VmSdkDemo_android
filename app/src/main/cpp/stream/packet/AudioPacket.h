/*
 * AudioPacket.h
 *
 *  Created on: 2016年9月28日
 *      Author: zhourui
 *      音频包
 *
 */

#ifndef AUDIOPACKET_H_
#define AUDIOPACKET_H_

#include "../../util/StreamPacket.h"
#include "PacketStreamType.h"

namespace Dream {

    class AudioPacket : public StreamPacket {

    public:
        AudioPacket() :
                StreamPacket(MSG_STREAM_AUDIO) {

        }

        virtual ~AudioPacket() = default;

        // ---------------------派生类重写开始---------------------
        // 无包体，无需覆盖父类方法
        // ---------------------派生类重写结束---------------------
    };

} /* namespace Dream */

#endif /* LOGINREQPACKET_H_ */
