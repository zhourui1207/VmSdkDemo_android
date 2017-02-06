/*
 * VideoPacket.h
 *
 *  Created on: 2016年9月28日
 *      Author: zhourui
 *      视频包
 *
 */

#ifndef VIDEOPACKET_H_
#define VIDEOPACKET_H_

#include "../../util/StreamPacket.h"
#include "PacketStreamType.h"

namespace Dream {

    class VideoPacket : public StreamPacket {

    public:
        VideoPacket() :
                StreamPacket(MSG_STREAM_VIDEO) {

        }

        virtual ~VideoPacket() = default;

        // ---------------------派生类重写开始---------------------
        // 无包体，无需覆盖父类方法
        // ---------------------派生类重写结束---------------------
    };

} /* namespace Dream */

#endif /* LOGINREQPACKET_H_ */
