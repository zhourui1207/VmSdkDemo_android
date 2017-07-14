//
// Created by zhou rui on 2017/7/13.
//

#ifndef DREAM_UDTSESSION_H
#define DREAM_UDTSESSION_H

#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include "UDTType.h"

namespace Dream {

    class UDTMultiplexer;

    class UDTSocket {

    public:
        const static unsigned UDT_VERSION = 4;  // 当前版本号4

    private:
        const char* TAG = "UDTSocket";
        const static unsigned INIT_RTT = 0;
        const static unsigned INIT_BUFFER_SIZE = 16;

    private:
        UDTSocket(SocketType socketType, ConnectionType connectionType,
                  std::shared_ptr<UDTMultiplexer> multiplexePtr, unsigned socketId,
                  const std::string &remoteAddr, unsigned short remotePort,
                  fUDTDataCallBack dataCallBack, fUDTConnectStatusCallBack statusCallBack,
                  void *pUser, unsigned maxPacketSize = DEFAULT_MTU,
                  unsigned maxWindowSize = DEFAULT_WINDOW_SIZE_MAX);

        virtual ~UDTSocket() = default;

        void sendSynPacket();

        void sendKeepAlivePacket();

        void sendAckPacket(unsigned seqNumber);

        void sendNakPacket();

        void sendAck2Packet();

        void sendShutdownPacket();

        void sendMdrPacket();

        void shutdownWithoutRemove();

        friend class UDTMultiplexer;

    public:
        void shutdown();  // 关闭

        void
        receiveData(const std::string &remote_address, unsigned short remote_port, const char *pBuf,
                    std::size_t len);

        std::string getRemoteAddr() {
            return _remoteAddr;
        }

        unsigned short getRemotePort() {
            return _remotePort;
        }

        void changeSocketStatus(SocketStatus socketStatus);

    private:
        void setup();  // 连接

        void run();

    private:
        SocketStatus _socketStatus;
        SocketType _socketType;
        unsigned _seqNumber;
        std::size_t _maxPacketSize;
        std::size_t _maxWindowSize;
        ConnectionType _connectionType;
        unsigned _cookie;
        std::shared_ptr<UDTMultiplexer> _multiplexerPtr;
        unsigned _socketId;
        std::string _remoteAddr;
        unsigned short _remotePort;
        fUDTDataCallBack _dataCallBack;
        fUDTConnectStatusCallBack _statusCallBack;
        void *_user;
        unsigned _addrBytes[4];

        bool _running;
        std::unique_ptr<std::thread> _threadPtr;
        std::mutex _mutex;

        bool _firstAck;
        unsigned _ackSeqNumber;
        unsigned _rttUs;
        unsigned _rttVariance;
        unsigned _bufferSize;
        unsigned _receiveRate;
        unsigned _linkCapacity;

    };

}

#endif //VMSDKDEMO_ANDROID_UDTSESSION_H
