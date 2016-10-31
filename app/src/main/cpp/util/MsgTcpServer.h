/*
 * MsgTcpServer.h
 *
 *  Created on: 2016年10月9日
 *      Author: zhourui
 */

#ifndef MSGTCPSERVER_H_
#define MSGTCPSERVER_H_

#include "AsioTcpServer.h"
#include "MsgPacket.h"

namespace Dream {

class MsgTcpServer {
public:
  MsgTcpServer() = delete;
  MsgTcpServer(ThreadPool& pool, const std::string& address, unsigned port) :
      _tcpServer(
          std::bind(&MsgTcpServer::receiveData, this, std::placeholders::_1,
              std::placeholders::_2), address, port), _pool(pool) {

    // 设置状态侦听器
    _tcpServer.setStatusListener(
        std::bind(&MsgTcpServer::onStatus, this, std::placeholders::_1));

    // 设置连接侦听器
    _tcpServer.setConnectListener(
        std::bind(&MsgTcpServer::onConnectStatus, this, std::placeholders::_1,
            std::placeholders::_2, std::placeholders::_3));

  }
  virtual ~MsgTcpServer() = default;

  // 启动
  bool startUp() {
    return _tcpServer.startUp();
  }
  // 关闭
  void shutDown() {
    _tcpServer.shutDown();
  }

  // 向某个客户端发送包
  void send(unsigned clientId, MsgPacket& msgPacket);

  // 向所有客户端发送包
  void sendAll(MsgPacket& msgPacket);

  // 派生类重写开始－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－

  // 子类用这个函数解析包,返回一个MsgPacket智能指针,不需要delete，底层会自动删除内存
  virtual std::shared_ptr<MsgPacket> newPacketPtr(unsigned msgType) = 0;

  // 子类继承这个用来接收包
  virtual void receive(unsigned clientId, std::shared_ptr<MsgPacket>& packetPtr) = 0;

  // 派生类处理自己的逻辑即可
  // 客户端连接成功
  virtual void onConnect(unsigned clientId, const std::shared_ptr<TcpClientSession>& sessionPtr) = 0;

  // 客户端连接断开
  virtual void onClose(unsigned clientId, const std::shared_ptr<TcpClientSession>& sessionPtr) = 0;

  // 派生类重写结束－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－

private:
  void receiveData(unsigned clientId, std::shared_ptr<PacketData>& dataPtr);
  void onStatus(AsioTcpServer::Status status);
  void onConnectStatus(unsigned clientId,
      const std::shared_ptr<TcpClientSession>& sessionPtr, bool isConnect);

private:
  AsioTcpServer _tcpServer;
  AsioTcpServer::Status _currentStatus;

  ThreadPool& _pool;
};

} /* namespace Dream */

#endif /* MSGTCPSERVER_H_ */
