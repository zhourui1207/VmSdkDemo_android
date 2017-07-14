//
// Created by zhou rui on 2017/7/13.
//

#ifndef DREAM_UDTMULTIPLEXER_H
#define DREAM_UDTMULTIPLEXER_H

/**
 * udt复用器，使得udt连接可以共享相同的端口号
 */
#include <bits/unique_ptr.h>
#include <map>
#include <set>
#include "../util/AsioUdpServer.h"
#include "packet/UDTBasePacket.h"
#include "UDTType.h"
#include "UDTSocket.h"

namespace Dream {

    /**
     * 由于一个端口对应一个复用器，那么一个复用器应该包含一个udp服务器
     */
    class UDTMultiplexer : std::enable_shared_from_this<UDTMultiplexer> {

    private:
        const char *TAG = "UDTMultiplexer";

    public:
        UDTMultiplexer(ConnectionType connectionType, const std::string &localAddr,
                       unsigned short localPort);

        virtual ~UDTMultiplexer();

        // 监听模式下使用
//        void setListener();

        bool startup();

        void shutdown();

        std::shared_ptr<UDTSocket> getSocketById(unsigned socketId);

        std::shared_ptr<UDTSocket>
        getSocketByAddrAndPort(const std::string &remoteAddr, unsigned short remotePort);

        // 主动创建连接使用
        std::shared_ptr<UDTSocket>
        addSocket(SocketType socketType, const std::string &remoteAddr, unsigned short remotePort,
                  fUDTDataCallBack dataCallBack, fUDTConnectStatusCallBack statusCallBack,
                  void *pUser, unsigned maxPacketSize = DEFAULT_MTU,
                  unsigned maxWindowSize = DEFAULT_WINDOW_SIZE_MAX);

        bool sendData(const std::string &remoteAddr, unsigned short remotePort, const char *pBuf,
                      std::size_t len);

    private:
        void
        receiveData(const std::string &remote_address, unsigned short remote_port, const char *pBuf,
                    std::size_t len);

        bool removeSocket(unsigned socketId);

        void removeAllSocket();

        bool getSocketId(unsigned &socketId);

        void recoverySocketId(int socketId);

        friend class UDTSocket;

    private:
        ConnectionType _connectionType;  // 连接类型 REGULAR：开启监听，等待客户端的连接，开启新的连接，被动创建连接；主动发起连接并同时等待目标连接，主动型
        std::unique_ptr<AsioUdpServer> _udpServerPtr;
        std::map<unsigned int, std::shared_ptr<UDTSocket>> _udtSocketMap;
        unsigned _udtSocketId;  // 名字叫做socketid，可以当作是sessionid
        std::set<unsigned> _recoveredSocketIds;  // 这个用来保存回收的id
        std::mutex _mutex;
    };

}


#endif //VMSDKDEMO_ANDROID_UDTMULTIPLEXER_H
