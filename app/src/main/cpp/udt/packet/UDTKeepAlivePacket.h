//
// Created by zhou rui on 2017/7/12.
//

#ifndef DREAM_UDTKEEPALIVEPACKET_H
#define DREAM_UDTKEEPALIVEPACKET_H

/**
 * udt保活包
 */
#include "UDTControlPacket.h"

namespace Dream {

    class UDTKeepAlivePacket : public UDTControlPacket {

    public:
        UDTKeepAlivePacket():
                UDTControlPacket(KEEP_ALIVE) {

        }

        virtual ~UDTKeepAlivePacket() = default;

    };

}

#endif //VMSDKDEMO_ANDROID_UDTSYNPACKET_H
