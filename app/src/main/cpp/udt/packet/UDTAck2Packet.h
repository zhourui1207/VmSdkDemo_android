//
// Created by zhou rui on 2017/7/12.
//

#ifndef DREAM_UDTACK2PACKET_H
#define DREAM_UDTACK2PACKET_H

/**
 * udt确认包的确认包
 */
#include "UDTControlPacket.h"

namespace Dream {

    class UDTAck2Packet : public UDTControlPacket {

    public:
        UDTAck2Packet(int32_t ackSeqNumber = 0) :
                UDTControlPacket(ACK2, 0, ackSeqNumber) {

        }

        virtual ~UDTAck2Packet() = default;
    };

}

#endif //VMSDKDEMO_ANDROID_UDTSYNPACKET_H
