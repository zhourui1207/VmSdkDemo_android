//
// Created by zhou rui on 2017/7/12.
//

#ifndef DREAM_UDTSHUTDOWNPACKET_H
#define DREAM_UDTSHUTDOWNPACKET_H

/**
 * udt关闭包
 */
#include "UDTControlPacket.h"

namespace Dream {

    class UDTShutdownPacket : public UDTControlPacket {

    public:
        UDTShutdownPacket():
                UDTControlPacket(SHUTDOWN) {

        }

        virtual ~UDTShutdownPacket() = default;

    };

}

#endif //VMSDKDEMO_ANDROID_UDTSYNPACKET_H
