//
// Created by zhou rui on 2017/7/13.
//

#ifndef DREAM_UDTTYPE_H
#define DREAM_UDTTYPE_H

#include <string>

const static std::size_t DEFAULT_MTU = 548;  // 576－20－8＝548
const static std::size_t DEFAULT_WINDOW_SIZE_MAX = 16;

enum ConnectionType {
    RENDEZVOUS = 0,  // p2p
    REGULAR = 1  // listener
};

enum SocketType {
    STREAM = 0,  // 流
    DGRAM = 1  // 报文
};

enum SocketStatus {
    SHUTDOWN = 0,  // 关闭状态
    LISTENER = 1,  // 监听状态
    CONNECTING = 2,  // 正在连接状态
    CONNECTED = 3 // 已连接
};

typedef void (*fUDTDataCallBack)(uint32_t uSocketId, char *pBuffer, std::size_t nLen, void *pUser);

typedef void (*fUDTConnectStatusCallBack)(uint32_t uSocketId, bool bIsConnected, void *pUser);

typedef void (*fUDTListCallBack)(uint32_t uSocketId, bool bIsConnected, void *pUser);


#endif //VMSDKDEMO_ANDROID_UDTTYPE_H
