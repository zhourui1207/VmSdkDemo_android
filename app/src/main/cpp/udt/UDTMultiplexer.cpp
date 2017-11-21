//
// Created by zhou rui on 2017/7/13.
//

#include "UDTMultiplexer.h"

namespace Dream {

    AddUDTRunnable::AddUDTRunnable(std::shared_ptr<UDTMultiplexer> multiplexerPtr, const std::string& remoteAddr, uint16_t remotePort, fUDTConnectStatusCallBack statusCallback, fUDTDataCallBack dataCallback, void* pUser, ConnectionMode connectionMode, uint32_t dstSocketId)
    : _multiplexerPtr(multiplexerPtr), _remoteAddr(remoteAddr), _remotePort(remotePort), _statusCallBack(statusCallback), _dataCallBack(dataCallback), _pUser(pUser), _connectionMode(connectionMode), _dstSocketId(dstSocketId) {
        
    }
    
    void AddUDTRunnable::run() {
        std::shared_ptr<UDTMultiplexer> multiplexerPtr = _multiplexerPtr.lock();
        if (multiplexerPtr.get()) {
            _socketPtr = multiplexerPtr->addSocketTask(_remoteAddr, _remotePort, _statusCallBack, _dataCallBack, _pUser, _connectionMode, _dstSocketId);
        }
    }

    UDTMultiplexerHandler::UDTMultiplexerHandler(LooperPtr looperPtr, std::shared_ptr<UDTMultiplexer> multiplexerPtr)
    : Handler(looperPtr), _multiplexerPtr(multiplexerPtr) {
        
    }

    UDTMultiplexerHandler::~UDTMultiplexerHandler() {

    }
    
    void UDTMultiplexerHandler::handleMessage(MessagePtr msgPtr) {
        std::shared_ptr<UDTMultiplexer> multiplexerPtr = _multiplexerPtr.lock();
        if (multiplexerPtr.get()) {
            switch (msgPtr->_what) {
                case UDTMultiplexer::MSG_TYPE_STARTUP:
                    multiplexerPtr->startupTask();
                    break;
                case UDTMultiplexer::MSG_TYPE_SHUTDOWN:
                    multiplexerPtr->shutdownTask();
                    break;
                case UDTMultiplexer::MSG_TYPE_RECEIVE_DATA:
                {
                    std::shared_ptr<UdpDataTask> udpDataPtr = std::dynamic_pointer_cast<UdpDataTask>(msgPtr->_objPtr);
                    multiplexerPtr->receiveDataTask(udpDataPtr->_remoteAddr, udpDataPtr->_remotePort, udpDataPtr->_pBuf, udpDataPtr->_len);
                    break;
                }
                case UDTMultiplexer::MSG_TYPE_REMOVE_SOCKET:
                    multiplexerPtr->removeSocketTask((std::dynamic_pointer_cast<RemoveSocketTask>(msgPtr->_objPtr))->_socketId);
                    break;
                case UDTMultiplexer::MSG_TYPE_REMOVE_SOCKET_WHITOUT_SHUTDOWN:
                    multiplexerPtr->removeSocketWithoutShutdownTask((std::dynamic_pointer_cast<RemoveSocketTask>(msgPtr->_objPtr))->_socketId);
                    break;
                default:
                    break;
            }
        }
    }

    UDTMultiplexer::UDTMultiplexer(const std::string &localAddr, uint16_t localPort,
                                   SocketType socketType, uint32_t maxPacketSize,
                                   uint32_t maxWindowSize,
                                   fUDTConnectStatusCallBack listenerStatusCallback,
                                   fUDTDataCallBack listenerDataCallback)
            : _running(false), _inited(false), _socketType(socketType), _maxPacketSize(maxPacketSize), _maxWindowSize(maxWindowSize),
              _listenerStatusCallback(listenerStatusCallback),
              _listenerDataCallback(listenerDataCallback), _udtSocketId(0) {
        _udpServerPtr.reset(new AsioUdpServer(
                std::bind(&UDTMultiplexer::receiveData, this, std::placeholders::_1,
                          std::placeholders::_2, std::placeholders::_3, std::placeholders::_4),
                localAddr, localPort));

    }

    UDTMultiplexer::~UDTMultiplexer() {
    
    }

    bool UDTMultiplexer::startup() {
        std::lock_guard<std::mutex> lock(_mutex);
        
        // 创建looper和handler
        if (_running) {
            LOGE(TAG, "Looper is starting!\n");
            return false;
        }
        
        if (_hPtr.get()) {
            LOGE(TAG, "Handler has inited!\n");
            return false;
        }
            
        _handlerThread.start();
        _hPtr = std::shared_ptr<Handler>(new UDTMultiplexerHandler(_handlerThread.getLooper(), shared_from_this()));
        _hPtr->sendEmptyMessage(MSG_TYPE_STARTUP);
        _running = true;
        return true;
    }
    
    void UDTMultiplexer::startupTask() {
        if (!_inited && _udpServerPtr.get() != nullptr) {
            _udpServerPtr->start_up();
            _inited = true;
        }
    }

    void UDTMultiplexer::shutdown() {
        std::lock_guard<std::mutex> lock(_mutex);

        if (_running) {
            _hPtr->sendEmptyMessage(MSG_TYPE_SHUTDOWN);
            _hPtr->getLooper()->quitSafely();
            _running = false;
        }
    }
    
    void UDTMultiplexer::shutdownTask() {
        if (_inited) {
            if (!_udtSocketMap.empty()) {
                removeAllSocket();
            }
            if (_udpServerPtr.get() != nullptr) {
                _udpServerPtr->shut_down();
            }
            _inited = false;
        }
    }

    std::shared_ptr<UDTSocket> UDTMultiplexer::getSocketById(uint32_t socketId) {
    
        std::shared_ptr<UDTSocket> udtSocketPtr;
        auto socketIt = _udtSocketMap.find(socketId);
        if (socketIt != _udtSocketMap.end()) {
            udtSocketPtr = socketIt->second;
        }

        return udtSocketPtr;
    }

    std::shared_ptr<UDTSocket> UDTMultiplexer::getSocketByAddrAndPort(const std::string &remoteAddr,
                                                                      uint16_t remotePort) {
        std::shared_ptr<UDTSocket> udtSocketPtr;
        auto socketIt = _udtSocketMap.begin();
        while (socketIt != _udtSocketMap.end()) {
            if (socketIt->second->getRemoteAddr() == remoteAddr &&
                socketIt->second->getRemotePort() == remotePort) {
                udtSocketPtr = socketIt->second;
                break;
            } else {
                ++socketIt;
            }
        }

        return udtSocketPtr;
    }
    
    std::shared_ptr<UDTSocket> UDTMultiplexer::getSocketByDstSocketId(uint32_t dstSocketId) {
        std::shared_ptr<UDTSocket> udtSocketPtr;
        auto socketIt = _udtSocketMap.begin();
        while (socketIt != _udtSocketMap.end()) {
            if (socketIt->second->getDstSocketId() == dstSocketId) {
                udtSocketPtr = socketIt->second;
                break;
            } else {
                ++socketIt;
            }
        }
        
        return udtSocketPtr;
    }
    
    std::shared_ptr<UDTSocket>
    UDTMultiplexer::addSocket(const std::string &remoteAddr,
                                  uint16_t remotePort,
                                  fUDTConnectStatusCallBack statusCallBack,
                                  fUDTDataCallBack dataCallBack,
                                  void *pUser, ConnectionMode connectionMode, uint32_t dstSocketId) {
        
        std::shared_ptr<UDTSocket> udtSocketPtr;
        
        std::lock_guard<std::mutex> lock(_mutex);
        if (_running) {
            RunnablePtr addSocketRunnableTask = RunnablePtr(new AddUDTRunnable(shared_from_this(), remoteAddr, remotePort, statusCallBack, dataCallBack, pUser, connectionMode, dstSocketId));
            if (_hPtr->runWithScissors(addSocketRunnableTask, ADD_SOCKET_TIMEOUT_MILLIS)) {
                udtSocketPtr = std::dynamic_pointer_cast<AddUDTRunnable>(addSocketRunnableTask)->_socketPtr.lock();
            }
        }
        
        return udtSocketPtr;
    }


    std::shared_ptr<UDTSocket>
    UDTMultiplexer::addSocketTask(const std::string &remoteAddr,
                              uint16_t remotePort,
                              fUDTConnectStatusCallBack statusCallBack,
                              fUDTDataCallBack dataCallBack,
                              void *pUser, ConnectionMode connectionMode, uint32_t dstSocketId) {
        LOGW(TAG, "Add socket task remoteAddr[%s] remotePort[%d] begin\n", remoteAddr.c_str(), remotePort);

        std::shared_ptr<UDTSocket> udtSocketPtr;
        uint32_t socketId = 0;

        if (getSocketId(socketId)) {
            udtSocketPtr.reset(
                    new UDTSocket(_socketType, connectionMode, shared_from_this(), socketId,
                                  remoteAddr, remotePort, dataCallBack, statusCallBack, pUser,
                                  dstSocketId, _maxPacketSize, _maxWindowSize));

            udtSocketPtr->setup();  // 发起连接请求
            _udtSocketMap.emplace(std::make_pair(udtSocketPtr->_socketId, udtSocketPtr));
            LOGW(TAG, "Add socket[%zd] remoteAddr[%s] remotePort[%d] success, socketId[%d], dstSocketId[%d]\n", udtSocketPtr.get(), remoteAddr.c_str(), remotePort, socketId, dstSocketId);
        }

        return udtSocketPtr;
    }

    bool UDTMultiplexer::removeSocket(uint32_t socketId) {
        std::lock_guard<std::mutex> lock(_mutex);
        
        if (_running) {
            MessagePtr mPtr = Message::obtain();
            mPtr->_what = MSG_TYPE_REMOVE_SOCKET;
            mPtr->_objPtr = ObjectPtr(new RemoveSocketTask(socketId));
            _hPtr->sendMessage(mPtr);
            return true;
        }
        
        return false;
    }
    
    void UDTMultiplexer::removeSocketTask(uint32_t socketId) {
        auto socketIt = _udtSocketMap.find(socketId);
        if (socketIt != _udtSocketMap.end()) {
            socketIt->second->shutdownWithoutRemove();
            _udtSocketMap.erase(socketIt);
            recoverySocketId(socketId);
        }
    }


    bool UDTMultiplexer::removeSocketWithoutShutdown(uint32_t socketId) {
        std::lock_guard<std::mutex> lock(_mutex);

        if (_running) {
            MessagePtr mPtr = Message::obtain();
            mPtr->_what = MSG_TYPE_REMOVE_SOCKET_WHITOUT_SHUTDOWN;
            mPtr->_objPtr = ObjectPtr(new RemoveSocketTask(socketId));
            _hPtr->sendMessage(mPtr);
            return true;
        }

        return false;
    }
    
    void UDTMultiplexer::removeSocketWithoutShutdownTask(uint32_t socketId) {
        auto socketIt = _udtSocketMap.find(socketId);
        if (socketIt == _udtSocketMap.end()) {
            return;
        }
        
        _udtSocketMap.erase(socketIt);
        recoverySocketId(socketId);
    }

    /**
     * 透传udp包
     * @param remoteAddr
     * @param remotePort
     * @param pBuf
     * @param len
     * @return
     */
    bool UDTMultiplexer::sendData(const std::string &remoteAddr, uint16_t remotePort,
                                  const char *pBuf, std::size_t len) {
        if (_udpServerPtr.get()) {
            return _udpServerPtr->send(remoteAddr, remotePort, pBuf, len);
        }
        return false;
    }

    void UDTMultiplexer::receiveData(const std::string &remote_address, uint16_t remote_port,
                                     const char *pBuf, std::size_t len) {
        if (_running) {
            MessagePtr msgPtr = Message::obtain();
            msgPtr->_what = MSG_TYPE_RECEIVE_DATA;
            msgPtr->_objPtr = ObjectPtr(new UdpDataTask(remote_address, remote_port, pBuf, len));
            _hPtr->sendMessage(msgPtr);
        }
    }
    
    void
    UDTMultiplexer::receiveDataTask(const std::string &remote_address, uint16_t remote_port, const char *pBuf,
                    std::size_t len) {
        // 分析dstSocketId
        uint32_t dstSocketId = 0;
        if (UDTBasePacket::decodeDstSocketIdStatic(pBuf, len, dstSocketId)) {  // 解析出dstSocketId才处理
            std::shared_ptr<UDTSocket> udtSocketPtr;
            
            if (dstSocketId == 0) {
                // 查询等待连接
                //                udtSocketPtr = getSocketByAddrAndPort(remote_address, remote_port);
                if (!udtSocketPtr.get() && UDTSocket::isConnectionRequest(pBuf, len) &&
                    (_listenerDataCallback || _listenerStatusCallback)) {  // 有监听
                    // 判断是否为连接包，如果是连接包，那么就创建一个新连接
                    uint32_t socketId = UDTSocket::socketId(pBuf, len);
                    
                    if (socketId > 0) {
                        udtSocketPtr = getSocketByDstSocketId(socketId);
                    }
                    
                    if (socketId > 0 && !udtSocketPtr.get()) {  // 如果用该dstSocketId没有创建连接，那么就创建新连接
                        udtSocketPtr = addSocketTask(remote_address, remote_port, _listenerStatusCallback,
                                                 _listenerDataCallback, nullptr, SERVER, socketId);
                    } else {
                        LOGW(TAG, "The dstSocketId[%d] has created a socket!\n", socketId);
                    }
                }
            } else {  // 找到相应的socket
                udtSocketPtr = getSocketById(dstSocketId);
            }
            
            if (udtSocketPtr.get()) {
                udtSocketPtr->receiveData(remote_address, remote_port, pBuf, len);
            } else {
                LOGW(TAG, "[%s:%d] can't find socket, socket id [%d]\n",
                     _udpServerPtr->local_address().c_str(), _udpServerPtr->local_port(),
                     dstSocketId);
            }
        }
    }

    void UDTMultiplexer::removeAllSocket() {
        std::unique_lock<std::mutex> lock(_mutex);

        auto socketIt = _udtSocketMap.begin();
        while (socketIt != _udtSocketMap.end()) {
            socketIt->second->shutdownWithoutRemove();
            ++socketIt;
        }

        _udtSocketMap.clear();
    }

    bool UDTMultiplexer::getSocketId(uint32_t &socketId) {
        if (!_recoveredSocketIds.empty()) {  // 如果有回收的，先用回收的
            socketId = *(_recoveredSocketIds.begin());
            _recoveredSocketIds.erase(socketId);
            return true;
        } else if (_udtSocketId < UINT_MAX) {
            socketId = ++_udtSocketId;
            return true;
        } else {  // id分配完了
            return false;
        }
    }

    void UDTMultiplexer::recoverySocketId(uint32_t socketId) {
        _recoveredSocketIds.emplace(socketId);
    }
}
