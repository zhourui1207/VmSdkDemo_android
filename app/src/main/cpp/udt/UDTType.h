//
// Created by zhou rui on 2017/7/13.
//

#ifndef DREAM_UDTTYPE_H
#define DREAM_UDTTYPE_H

#include <string>

const static std::size_t DEFAULT_MTU = 576;  // 576－20－8＝548
const static std::size_t DEFAULT_WINDOW_SIZE_MAX = 16;

enum ConnectionMode {
    RENDEZVOUS = 0,  // p2p
    SERVER = 1,  // listener
    CLIENT = 2
};

enum SocketType {
    STREAM = 0,  // 流
    DGRAM = 1  // 报文
};

enum SocketStatus {
    UNCONNECTED = 0,  // 未连接
    CONNECTING = 1,  // 正在连接状态
    CONNECTED = 2, // 已连接
    SHUTDOWN = 3  // 关闭状态
};

typedef void (*fUDTDataCallBack)(uint32_t uSocketId, const char *pBuffer, std::size_t nLen, void *pUser);

typedef void (*fUDTConnectStatusCallBack)(uint32_t uSocketId, bool bIsConnected, void *pUser);

typedef void (*fUDTListCallBack)(uint32_t uSocketId, bool bIsConnected, void *pUser);

// 拥塞控制回调
// 接收到ACK包
//typedef void (*fUDTOnACK)(bool *slowStartPhase, size_t *CWND, int32_t *SND, uint32_t packetArrivalRate, uint32_t estimatedLinkCapacity, int32_t RTT, uint32_t PS, );
// 接收到NAK包
//typedef void (*fUDTOnLOSS)();


#endif //VMSDKDEMO_ANDROID_UDTTYPE_H
