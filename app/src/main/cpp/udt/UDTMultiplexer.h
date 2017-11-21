//
// Created by zhou rui on 2017/7/13.
//

#ifndef DREAM_UDTMULTIPLEXER_H
#define DREAM_UDTMULTIPLEXER_H

/**
 * udt复用器，使得udt连接可以共享相同的端口号
 */
#include <map>
#include <memory>
#include <set>
#include "../util/AsioUdpServer.h"
#include "packet/UDTBasePacket.h"
#include "UDTType.h"
#include "UDTSocket.h"
#include "../util/Handler.h"
#include "../util/Object.h"
#include "../util/HandlerThread.h"
#include "../util/Runnable.h"

namespace Dream {

    class RemoveSocketTask : public Object {
    public:
        RemoveSocketTask(uint32_t socketId)
        : _socketId(socketId) {
        }
    
        uint32_t _socketId;
    };
    
    class UdpDataTask : public Object {
    public:
        UdpDataTask(const std::string& remoteAddr, uint16_t remotePort, const char* pBuf, std::size_t len)
        : _remoteAddr(remoteAddr), _remotePort(remotePort), _pBuf(nullptr), _len(len) {
            if (pBuf && _len > 0) {
                _pBuf = new char[_len];
                memcpy(_pBuf, pBuf, _len);
            }
        }
        
        virtual ~UdpDataTask() {
            if (_pBuf) {
                delete [] _pBuf;
                _pBuf = nullptr;
            }
        }
    
        std::string _remoteAddr;
        uint16_t _remotePort;
        char* _pBuf;
        std::size_t _len;
    };
    
    class AddUDTRunnable : public Runnable {
    public:
        AddUDTRunnable() = delete;
    
        AddUDTRunnable(std::shared_ptr<UDTMultiplexer> multiplexerPtr, const std::string& remoteAddr, uint16_t remotePort, fUDTConnectStatusCallBack statusCallback, fUDTDataCallBack dataCallback, void* pUser, ConnectionMode connectionMode, uint32_t dstSocketId);
        
        virtual ~AddUDTRunnable() = default;
        
        virtual void run() override;
        
        std::weak_ptr<UDTSocket> _socketPtr;
    
    private:
        std::weak_ptr<UDTMultiplexer> _multiplexerPtr;
        std::string _remoteAddr;
        uint16_t _remotePort;
        fUDTConnectStatusCallBack _statusCallBack;
        fUDTDataCallBack _dataCallBack;
        void* _pUser;
        ConnectionMode _connectionMode;
        uint32_t _dstSocketId;
    };

    class UDTMultiplexerHandler : public Handler {
    public:
        UDTMultiplexerHandler() = delete;
    
        UDTMultiplexerHandler(LooperPtr looperPtr, std::shared_ptr<UDTMultiplexer> multiplexerPtr);
        
        virtual ~UDTMultiplexerHandler();
        
        virtual void handleMessage(MessagePtr msgPtr) override;
        
    private:
        std::weak_ptr<UDTMultiplexer> _multiplexerPtr;
    };

    /**
     * 由于一个端口对应一个复用器，那么一个复用器应该包含一个udp服务器
     */
    class UDTMultiplexer : public std::enable_shared_from_this<UDTMultiplexer> {

    private:
        const char *TAG = "UDTMultiplexer";
        const static int64_t ADD_SOCKET_TIMEOUT_MILLIS = 1000;

    public:
        // 消息类型定义
        const static int MSG_TYPE_STARTUP = 1;  // 启动
        const static int MSG_TYPE_SHUTDOWN = 2;  // 关闭
        const static int MSG_TYPE_RECEIVE_DATA = 3;  // 接收数据
        const static int MSG_TYPE_REMOVE_SOCKET = 98;
        const static int MSG_TYPE_REMOVE_SOCKET_WHITOUT_SHUTDOWN = 99;  // 自动移除socket
        
        
        UDTMultiplexer(const std::string &localAddr, uint16_t localPort = 0,
                       SocketType socketType = STREAM,
                       uint32_t maxPacketSize = DEFAULT_MTU,
                       uint32_t maxWindowSize = DEFAULT_WINDOW_SIZE_MAX,
                       fUDTConnectStatusCallBack listenerStatusCallback = nullptr,
                       fUDTDataCallBack listenerDataCallback = nullptr);

        virtual ~UDTMultiplexer();

        // 监听模式下使用
//        void setListener();

        bool startup();

        void shutdown();

        bool sendData(const std::string &remoteAddr, uint16_t remotePort, const char *pBuf,
                      std::size_t len);

        const std::string localAddress() const {
            return _udpServerPtr->local_address();
        }

        uint16_t localPort() const {
            return _udpServerPtr->local_port();
        }
        
        // 主动创建连接使用
        std::shared_ptr<UDTSocket> addSocket(const std::string &remoteAddr, uint16_t remotePort,
                                             fUDTConnectStatusCallBack statusCallBack, fUDTDataCallBack dataCallBack,
                                             void *pUser, ConnectionMode connectionMode = RENDEZVOUS, uint32_t dstSocketId = 0);

    private:
        friend class UDTMultiplexerHandler;
        
        friend class UDTSocket;
        
        friend class AddUDTRunnable;
        
        void startupTask();
        
        void shutdownTask();
    
        void
        receiveData(const std::string &remote_address, uint16_t remote_port, const char *pBuf,
                    std::size_t len);
        
        void
        receiveDataTask(const std::string &remote_address, uint16_t remote_port, const char *pBuf,
                    std::size_t len);
        
        std::shared_ptr<UDTSocket> getSocketById(uint32_t socketId);
        
        std::shared_ptr<UDTSocket>
        getSocketByAddrAndPort(const std::string &remoteAddr,uint16_t remotePort);
        
        std::shared_ptr<UDTSocket> getSocketByDstSocketId(uint32_t dstSocketId);
        
        std::shared_ptr<UDTSocket> addSocketTask(const std::string &remoteAddr, uint16_t remotePort,
                  fUDTConnectStatusCallBack statusCallBack, fUDTDataCallBack dataCallBack,
                  void *pUser, ConnectionMode connectionMode = RENDEZVOUS, uint32_t dstSocketId = 0);

        bool removeSocket(uint32_t socketId);
        
        void removeSocketTask(uint32_t socketId);

        bool removeSocketWithoutShutdown(uint32_t socketId);
        
        void removeSocketWithoutShutdownTask(uint32_t socketId);

        void removeAllSocket();

        bool getSocketId(uint32_t &socketId);

        void recoverySocketId(uint32_t socketId);

        bool _running;
        bool _inited;
        const SocketType _socketType;
        uint32_t _maxPacketSize;
        uint32_t _maxWindowSize;
        const fUDTConnectStatusCallBack  _listenerStatusCallback;
        const fUDTDataCallBack _listenerDataCallback;
        std::unique_ptr<AsioUdpServer> _udpServerPtr;
        std::map<uint32_t, std::shared_ptr<UDTSocket>> _udtSocketMap;
        uint32_t _udtSocketId;  // 名字叫做socketid，可以当作是sessionid
        std::set<uint32_t> _recoveredSocketIds;  // 这个用来保存回收的id
        std::mutex _mutex;
        HandlerThread _handlerThread;
        HandlerPtr _hPtr;
    };

}


#endif //VMSDKDEMO_ANDROID_UDTMULTIPLEXER_H
