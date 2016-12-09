/*
 * AsioTcpServer.h
 *
 *  Created on: 2016年9月27日
 *      Author: zhourui
 *      基于asio的tcp服务端
 */

#ifndef ASIOTCPSERVER_H_
#define ASIOTCPSERVER_H_

#include <atomic>
#include <boost/asio.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>

#include "PacketData.h"
#include "Singleton.h"
#include "ThreadPool.h"

namespace Dream {

class TcpClientSessions;
// tcp客户端session
class TcpClientSession: public std::enable_shared_from_this<TcpClientSession> {
  using SendQueue = std::queue<std::shared_ptr<PacketData>>;
private:
  static const unsigned DEFAULT_READ_SIZE = 100 * 1024;  // 接收时的默认缓存大小
  static const unsigned DEFAULT_WRITE_SIZE = 100 * 1024;  // 发送时的默认缓存大小

public:
  TcpClientSession(boost::asio::ip::tcp::socket socket,
      TcpClientSessions& sessions,
      const std::function<
          void(unsigned clientId, std::shared_ptr<PacketData>& dataPtr)>& callback,
      const std::function<
          void(unsigned clientId, const std::shared_ptr<TcpClientSession>& sessionPtr,
          bool isConnect)>& connectListener,
      unsigned receiveSize = DEFAULT_READ_SIZE, unsigned sendSize =
          DEFAULT_WRITE_SIZE);

  virtual ~TcpClientSession();

  bool startUp();

  void shutDown();

  void send(const std::shared_ptr<PacketData>& dataPtr);

private:
  void doRead();
  void doWrite();

private:
  unsigned _clientId;
  boost::asio::ip::tcp::socket _socket;
  TcpClientSessions& _sessions;
  std::function<void(unsigned clientId, std::shared_ptr<PacketData>& dataPtr)> _callback;
  std::function<void(unsigned clientId, const std::shared_ptr<TcpClientSession>& sessionPtr, bool isConnect)> _connectListener;
  unsigned _receiveSize;
  unsigned _sendSize;

  char* _receive;  // 接收数据缓存
  SendQueue _sendQueue;  // 发送队列
};

class SessionIdManager: public Singleton<SessionIdManager> {

public:
  SessionIdManager() : _id(0) {

  }

  virtual ~SessionIdManager() = default;

  // 获得sessionid 从 1 - uint_max
  bool getSessionId(unsigned& sessionId) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_sessions.size() > 0) {  // 如果有回收的，先用回收的
      sessionId = *(_sessions.begin());
      _sessions.erase(sessionId);
      return true;
    } else if (_id < UINT_MAX) {
      sessionId = ++_id;
      return true;
    } else {  // id分配完了
      return false;
    }
  }

  // 还回sessionid
  void recoverySessionId(int sessionId) {
    std::lock_guard<std::mutex> lock(_mutex);
    _sessions.emplace(sessionId);
  }

private:
  unsigned _id;
  std::set<unsigned> _sessions;  // 这个用来保存回收的id
  std::mutex _mutex;
};

class TcpClientSessions {
public:
  TcpClientSessions() = default;
  virtual ~TcpClientSessions() = default;

  bool addSession(const std::shared_ptr<TcpClientSession>& sessionPtr,
      unsigned& clientId) {
    std::lock_guard<std::mutex> lock(_mutex);
    bool ret = SessionIdManager::getInstance()->getSessionId(clientId);
    if (ret) {
      _sessions.emplace(std::make_pair(clientId, sessionPtr));
    }
    return ret;
  }

  void removeSession(unsigned clientId) {
    std::lock_guard<std::mutex> lock(_mutex);
    _sessions.erase(clientId);
    SessionIdManager::getInstance()->recoverySessionId(clientId);
  }

  void sendAll(const std::shared_ptr<PacketData>& dataPtr) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_sessions.size() > 0) {
      auto sessionPtrIt = _sessions.begin();
      for (; sessionPtrIt != _sessions.end(); ++sessionPtrIt) {
        sessionPtrIt->second->send(dataPtr);
      }
    }
  }

  void send(unsigned clientId, const std::shared_ptr<PacketData>& dataPtr) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto sessionPtr = getSession(clientId);
    if (sessionPtr.get() != nullptr) {
      sessionPtr->send(dataPtr);
    }
  }

private:
  std::shared_ptr<TcpClientSession> getSession(unsigned clientId) {
    std::shared_ptr<TcpClientSession> retPtr(nullptr);
    auto sessionPtrIt = _sessions.find(clientId);
    if (sessionPtrIt != _sessions.end()) {
      retPtr = sessionPtrIt->second;
    }
    return retPtr;
  }

private:
  std::map<unsigned, std::shared_ptr<TcpClientSession>> _sessions;
  std::mutex _mutex;
};

class AsioTcpServer {
  using SendQueue = std::queue<std::shared_ptr<PacketData>>;
public:
  enum Status {
    NO_INIT = 0,  // 未接受
    INITING,  // 正在初始化
    INITED,  // 初始化完成
    UNINITING,  // 正在反初始化
    CLOSEING,
    CLOSED
  };

private:
  static const unsigned DEFAULT_READ_SIZE = 10 * 1024;  // 接收时的默认缓存大小
  static const unsigned DEFAULT_WRITE_SIZE = 10 * 1024;  // 发送时的默认缓存大小

public:
  // 指定ip地址的构造
  AsioTcpServer(
      const std::function<
          void(unsigned clientId, std::shared_ptr<PacketData>& dataPtr)>& callback,
      const std::string& address, unsigned port,
      unsigned receiveSize = DEFAULT_READ_SIZE, unsigned sendSize =
          DEFAULT_WRITE_SIZE);

  // 使用默认ip地址的构造
  AsioTcpServer(
      const std::function<
          void(unsigned clientId, std::shared_ptr<PacketData>& dataPtr)>& callback,
      unsigned port, unsigned receiveSize = DEFAULT_READ_SIZE,
      unsigned sendSize = DEFAULT_WRITE_SIZE);

  virtual ~AsioTcpServer();

  // 设置状态侦听器
  void setStatusListener(const std::function<void(Status)>& statusListener) {
    _statusListener = statusListener;
  }
  // 设置连接侦听器
  void setConnectListener(
      const std::function<
          void(unsigned clientId, const std::shared_ptr<TcpClientSession>& sessionPtr,
          bool isConnect)>& connectListener) {
    _connectListener = connectListener;
  }
  bool startUp();
  void shutDown();
  void sendAll(const std::shared_ptr<PacketData>& dataPtr);
  void send(unsigned clientId, const std::shared_ptr<PacketData>& dataPtr);

private:
  bool doInit();
  void doAccept();

  void run();
  void close();

private:
  std::function<void(unsigned clientId, std::shared_ptr<PacketData>& dataPtr)> _callback; // 接收到包的回调
  std::string _address;  // 服务端地址
  unsigned _port;  // 服务端端口
  unsigned _receiveSize;
  unsigned _sendSize;
  std::atomic<Status> _currentStatus;
  std::function<void(Status)> _statusListener;  // 状态侦听器
  std::function<
      void(unsigned clientId, const std::shared_ptr<TcpClientSession>& sessionPtr,
          bool isConnect)> _connectListener; // 连接侦听器

  boost::asio::io_service _ioService;  // 异步网络服务
  std::unique_ptr<boost::asio::ip::tcp::acceptor> _acceptorPtr;
  std::unique_ptr<boost::asio::ip::tcp::socket> _socketPtr;  // 套接字智能指针
  std::unique_ptr<std::thread> _threadPtr;  // 线程智能指针
  boost::asio::ip::tcp::endpoint _endpoint;

  char* _receive;  // 接收数据缓存
  SendQueue _sendQueue;  // 发送队列

  TcpClientSessions _sessions;
};

} /* namespace Dream */

#endif /* ASIOTCPSERVER_H_ */
