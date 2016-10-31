/*
 * AsioTcpServer.cpp
 *
 *  Created on: 2016年9月27日
 *      Author: zhourui
 */

#include <stdio.h>

#include "AsioTcpServer.h"

namespace Dream {

using boost::asio::ip::tcp;

TcpClientSession::TcpClientSession(tcp::socket socket,
    TcpClientSessions& sessions,
    const std::function<
        void(unsigned clientId, std::shared_ptr<PacketData>& dataPtr)>& callback,
    const std::function<
        void(unsigned clientId, const std::shared_ptr<TcpClientSession>& sessionPtr,
        bool isConnect)>& connectListener,
    unsigned receiveSize, unsigned sendSize) :
    _clientId(0), _socket(std::move(socket)), _sessions(sessions), _callback(callback), _connectListener(connectListener),
    _receiveSize(receiveSize), _sendSize(sendSize), _receive(nullptr) {
  _receive = new char[receiveSize];
}

TcpClientSession::~TcpClientSession() {
  if (_receive != nullptr) {
    delete[] _receive;
  }
}

bool TcpClientSession::startUp() {
  bool ret = _sessions.addSession(shared_from_this(), _clientId);
  if (ret) {
    _connectListener(_clientId, shared_from_this(), true);
    doRead();
  }
  return ret;
}

void TcpClientSession::shutDown() {
  _connectListener(_clientId, shared_from_this(), false);
  _sessions.removeSession(_clientId);
}

void TcpClientSession::send(const std::shared_ptr<PacketData>& dataPtr) {
  bool sending = !_sendQueue.empty();
  _sendQueue.emplace(dataPtr);
  if (!sending) {
    doWrite();
  }
}

void TcpClientSession::doRead() {
  auto self(shared_from_this());
  _socket.async_read_some(boost::asio::buffer(_receive, _receiveSize),
      [this, self](boost::system::error_code ec, std::size_t length) {
        if (!ec && length > 0) {
          std::shared_ptr<PacketData> dataPtr = std::make_shared<PacketData>(length, _receive);
          _callback(_clientId, dataPtr);
          // _pool.addTask(std::bind(_callback, _clientId, dataPtr));
          doRead();
        } else if (ec) {
          printf("读取数据异常, 连接断开:%s\n", ec.message().c_str());
          shutDown();
        }
      });
}

void TcpClientSession::doWrite() {
  printf("开始写数据...\n");
  boost::asio::async_write(_socket,
      boost::asio::buffer(_sendQueue.front()->data(),
          _sendQueue.front()->length()),
      [this](boost::system::error_code ec, std::size_t /*length*/) {
        if (!ec) {
          _sendQueue.pop();
          if (!_sendQueue.empty()) {
            doWrite();
          }
        } else {
          printf("写数据异常, 连接断开:%s\n", ec.message().c_str());
          shutDown();
        }
      });
}

AsioTcpServer::AsioTcpServer(
    const std::function<
        void(unsigned clientId, std::shared_ptr<PacketData>& dataPtr)>& callback,
    const std::string& address, unsigned port,
    unsigned receiveSize, unsigned sendSize) :
    _callback(callback), _address(address), _port(port), _receiveSize(
        receiveSize), _sendSize(sendSize), _currentStatus(NO_INIT), _statusListener(
        [](int status)->void {printf("当前状态[%d]\n", status);}), _connectListener(
        [](unsigned clientId, const std::shared_ptr<TcpClientSession>&, bool isConnect)->void {printf("[%d]连接状态改变, isConnect[%d]\n", clientId, isConnect);}), _receive(nullptr) {
  _receive = new char[receiveSize];
}

AsioTcpServer::AsioTcpServer(
    const std::function<
        void(unsigned clientId, std::shared_ptr<PacketData>& dataPtr)>& callback,
    unsigned port, unsigned receiveSize, unsigned sendSize) :
    AsioTcpServer(callback, "", port, receiveSize, sendSize) {
}

AsioTcpServer::~AsioTcpServer() {
  if (_receive != nullptr) {
    delete[] _receive;
  }
}

bool AsioTcpServer::startUp() {
  if (_currentStatus.load() != NO_INIT) {
    return false;
  }

  // 初始化
  if (!doInit()) {
    return false;
  }

  // 开始接收
  doAccept();

  _threadPtr.reset(new std::thread(&AsioTcpServer::run, this));
  _threadPtr->detach();

  return true;
}

void AsioTcpServer::shutDown() {
  close();
}

void AsioTcpServer::sendAll(const std::shared_ptr<PacketData>& dataPtr) {
  _sessions.sendAll(dataPtr);
}

void AsioTcpServer::send(unsigned clientId,
    const std::shared_ptr<PacketData>& dataPtr) {
  _sessions.send(clientId, dataPtr);
}

bool AsioTcpServer::doInit() {
  if (_currentStatus.load() == INITING) {
    return false;
  }

  printf("AsioTcpServer正在初始化......\n");
  _currentStatus.store(INITING);
  _statusListener(_currentStatus);

  try {
    if (_address.length() == 0) {
      _endpoint = tcp::endpoint(tcp::v4(), _port);
    } else {
      _endpoint = tcp::endpoint(
          boost::asio::ip::address_v4::from_string(_address), _port);
    }
    _acceptorPtr.reset(new tcp::acceptor(_ioService, _endpoint));
    _socketPtr.reset(new tcp::socket(_ioService));
  } catch (boost::system::system_error & e) {
    printf("AsioTcpServer地址或端口不可用address=[%s],port=[%d],[%s]\n", _address.c_str(), _port,
        e.what());
    _currentStatus.store(INITED);
    _statusListener(_currentStatus);
    return false;
  }

  _currentStatus.store(INITED);
  _statusListener(_currentStatus);
  return true;
}

void AsioTcpServer::doAccept() {
  _acceptorPtr->async_accept(*(_socketPtr.get()),
      [this](boost::system::error_code ec) {
        if (!ec) {
          if(!std::make_shared<TcpClientSession>(std::move(*(_socketPtr.get())), _sessions, _callback, _connectListener, _receiveSize, _sendSize)->startUp()) {
            printf("接收客户端失败，可能是连结数过多！\n");
          }
        }
        doAccept();
      });
}

void AsioTcpServer::run() {
  _ioService.run();
}
void AsioTcpServer::close() {
  if (_currentStatus.load() == CLOSEING || _currentStatus.load() == CLOSED) {
    return;
  }
  _currentStatus.store(CLOSEING);
  _statusListener(_currentStatus);
  _socketPtr->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
  _socketPtr->close();  // 关闭套接字
  _currentStatus.store(CLOSED);
  _statusListener(_currentStatus);
}

} /* namespace Dream */
