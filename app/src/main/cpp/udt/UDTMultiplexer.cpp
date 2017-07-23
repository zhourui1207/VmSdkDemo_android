//
// Created by zhou rui on 2017/7/13.
//

#include "UDTMultiplexer.h"

namespace Dream {

    UDTMultiplexer::UDTMultiplexer(ConnectionType connectionType, const std::string &localAddr,
                                   uint16_t localPort)
            : _connectionType(connectionType), _udtSocketId(0) {
        _udpServerPtr.reset(new AsioUdpServer(
                std::bind(&UDTMultiplexer::receiveData, this, std::placeholders::_1,
                          std::placeholders::_2, std::placeholders::_3, std::placeholders::_4),
                localAddr, localPort));
    }

    UDTMultiplexer::~UDTMultiplexer() {
        shutdown();
    }

    bool UDTMultiplexer::startup() {
        if (_udpServerPtr.get() != nullptr) {
            return _udpServerPtr->start_up();
        }
        return false;
    }

    void UDTMultiplexer::shutdown() {
        if (!_udtSocketMap.empty()) {
            removeAllSocket();
        }
        if (_udpServerPtr.get() != nullptr) {
            _udpServerPtr->shut_down();
        }
    }

    std::shared_ptr<UDTSocket> UDTMultiplexer::getSocketById(uint32_t socketId) {
        std::unique_lock<std::mutex> lock(_mutex);

        std::shared_ptr<UDTSocket> udtSocketPtr;
        auto socketIt = _udtSocketMap.find(socketId);
        if (socketIt != _udtSocketMap.end()) {
            udtSocketPtr = socketIt->second;
        }

        return udtSocketPtr;
    }

    std::shared_ptr<UDTSocket> UDTMultiplexer::getSocketByAddrAndPort(const std::string &remoteAddr,
                                                                      uint16_t remotePort) {
        std::unique_lock<std::mutex> lock(_mutex);

        std::shared_ptr<UDTSocket> udtSocketPtr;
        auto socketIt = _udtSocketMap.begin();
        while (socketIt != _udtSocketMap.end()) {
            if (socketIt->second->getRemoteAddr() == remoteAddr &&
                socketIt->second->getRemotePort() == remotePort) {
                udtSocketPtr = socketIt->second;
                break;
            }
        }

        return udtSocketPtr;
    }

    std::shared_ptr<UDTSocket>
    UDTMultiplexer::addSocket(SocketType socketType, const std::string &remoteAddr,
                              uint16_t remotePort,
                              fUDTDataCallBack dataCallBack,
                              fUDTConnectStatusCallBack statusCallBack,
                              void *pUser, uint32_t maxPacketSize, uint32_t maxWindowSize) {
        std::unique_lock<std::mutex> lock(_mutex);

        std::shared_ptr<UDTSocket> udtSocketPtr;
        if (_connectionType == RENDEZVOUS) {  // p2p模式
            unsigned socketId = 0;
            if (getSocketId(socketId)) {
                udtSocketPtr.reset(
                        new UDTSocket(socketType, _connectionType, shared_from_this(), socketId,
                                      remoteAddr, remotePort, dataCallBack, statusCallBack, pUser,
                                      maxPacketSize, maxWindowSize));

                udtSocketPtr->setup();  // 发起连接请求
                _udtSocketMap.emplace(std::make_pair(udtSocketPtr->_socketId, udtSocketPtr));
            }
        } else {
            LOGE(TAG, "Add socket must be in Rendezvous type!\n");
        }

        return udtSocketPtr;
    }

    bool UDTMultiplexer::removeSocket(uint32_t socketId) {
        std::unique_lock<std::mutex> lock(_mutex);

        auto socketIt = _udtSocketMap.find(socketId);
        if (socketIt == _udtSocketMap.end()) {
            return false;
        }

        socketIt->second->shutdown();
        _udtSocketMap.erase(socketIt);
        recoverySocketId(socketId);

        return true;
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
        if (_udpServerPtr.get() != nullptr) {
            return _udpServerPtr->send(remoteAddr, remotePort, pBuf, len);
        }
        return false;
    }

    void UDTMultiplexer::receiveData(const std::string &remote_address, uint16_t remote_port,
                                     const char *pBuf, std::size_t len) {
        // 分析dstSocketId
        uint32_t dstSocketId = 0;
        if (UDTBasePacket::decodeDstSocketIdStatic(pBuf, len, dstSocketId)) {  // 解析出dstSocketId才处理
            std::shared_ptr<UDTSocket> udtSocketPtr;

            if (dstSocketId == 0) {
                if (_connectionType == REGULAR) {  // 监听模式
                    // todo：创建新的连接
                } else {
                    // 查询等待连接
                    udtSocketPtr = getSocketByAddrAndPort(remote_address, remote_port);
                }
            } else {  // 找到相应的socket
                udtSocketPtr = getSocketById(dstSocketId);
            }

            if (udtSocketPtr.get() != nullptr) {
                udtSocketPtr->receiveData(remote_address, remote_port, pBuf, len);
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
