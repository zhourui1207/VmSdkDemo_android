/*
 * AsioTcpClient.h
 *
 *  Created on: 2016年9月27日
 *      Author: zhourui
 *      基于asio的tcp客户端
 */

#ifndef ASIOTCPCLIENT_H_
#define ASIOTCPCLIENT_H_

#include <atomic>
#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <queue>

#include "Noncopyable.h"
#include "PacketData.h"
#include "ThreadPool.h"
#include "Timer.h"

namespace Dream {

// 传入类型请继承BasePacket
    class AsioTcpClient : public Noncopyable {
        using SendQueue = std::queue<std::shared_ptr<PacketData>>;

    public:
        enum Status {
            NO_CONNECT = 0,  // 无连接
            INITING,  // 正在初始化
            CONNECTING,  // 正在连接
            CONNECTED,  // 已连接
            CLOSEING,  // 正在关闭
            CLOSED  // 已关闭
        };

    private:
        static const bool RECEIVE_MODE = true;  // 默认为自由接收模式
        static const unsigned RECONNECT_INTERVAL = 10000;  // 默认重连时间间隔
        static const unsigned DEFAULT_READ_SIZE = 10 * 1024;  // 接收时的默认缓存大小
        static const unsigned DEFAULT_WRITE_SIZE = 10 * 1024;  // 发送时的默认缓存大小

    public:
        AsioTcpClient() = delete;

        AsioTcpClient(
                std::function<void(const char *pBuf, std::size_t len)> callback,
                const std::string &address, unsigned short port,
                unsigned receiveSize = DEFAULT_READ_SIZE, unsigned sendSize =
        DEFAULT_WRITE_SIZE, unsigned reconnectInterval = RECONNECT_INTERVAL);

        virtual ~AsioTcpClient();

        // 设置状态侦听器
        void setStatusListener(std::function<void(Status)> statusListener) {
            _statusListener = statusListener;
        }

        // 移除状态侦听
        void removeStatusListener() {
            _statusListener = nullptr;
        }

        // 移除回调，当接收回调函数的对象析构时，调用此函数
        void removeCallback() {
            _callback = nullptr;
        }

        // 设置读取长度回调，此函数必须在startup之前调用，不然会在下一次重连后才生效
        // (通常在意义好协议，并且需要解包处理时调用，若作为转发服务，不需要调用该函数)
        void setReadLengthCallback(std::function<unsigned()> readLengthCallback) {
            _readLengthCallback = readLengthCallback;
        }

        void removeReadLengthCallback() {
            _readLengthCallback = nullptr;
        }

        unsigned receiveSize() {
            return _receiveSize;
        }

        bool startUp();

        void shutDown(bool isIoServiceThread = false);  // isIoServiceThread是否是ioService线程调用

        // 返回true表示成功加发送数据添加到发送缓存队列中，并不表示发送出去；返回false一般是未连接的情况
        bool send(const std::shared_ptr<PacketData> &dataPtr);

        bool send(const char* buf, std::size_t len);

        // 同步发送
        bool sendSync(const std::shared_ptr<PacketData> &dataPtr);

        bool sendSync(const char* buf, std::size_t len);

        const std::string remoteAddress() const {
            return _address;
        }

        unsigned short remotePort() const {
            return _port;
        }

        const std::string localAddress() const {
            if (_socketPtr.get() == nullptr) {
                return "";
            }
            return _socketPtr->local_endpoint().address().to_string();
        }

        unsigned short localPort() {
            if (_socketPtr.get() == nullptr) {
                return 0;
            }
            return _socketPtr->local_endpoint().port();
        }

        void reconnect() {
            doReconnect();
        }

    private:
        bool doInit();

        void doConnect();

        void doReconnect();

        void doReadByLength();  // 通过上层给予的长度进行读取数据
        void doRead();

        void doWrite();

        void doOnStatus(Status status) {
            if (_statusListener) {
                _statusListener(status);
            }
        }

        unsigned getReadLength() {
            if (_readLengthCallback) {
                return _readLengthCallback();
            } else {
                return 0;
            }
        }

        void run();

        void close(bool isIoServiceThread);

    private:
        // 接收到数据的回调，如果没有设置readlengthcallback，那么数据长度是不定且无边界的，需要接收者自己来解析分包、拼包；
        // 如果设置过readlengthcallback，那么接收到的包将会，严格按照上层希望读取的长度来读取数据
        std::function<void(const char *pBuf, std::size_t len)> _callback;
        std::string _address;  // 服务端地址
        unsigned short _port;  // 服务端端口

        std::function<unsigned()> _readLengthCallback;  // 如果是按协议接收模式，在每次读取数据之前，将调用此函数获取需要读取多少长度的数据

        unsigned _receiveSize;
        unsigned _sendSize;
        unsigned _reconnectInterval;  // 重连间隔
        std::atomic<Status> _currentStatus;
        std::function<void(Status)> _statusListener;  // 状态侦听器

        // boost::asio::io_service _ioService;  // 异步网络服务
        // 如果要实现关闭后能在再启动运行，必须重新新建ioservice，所以这里使用指针
        std::unique_ptr<boost::asio::io_service> _ioServicePtr;
        std::unique_ptr<boost::asio::ip::tcp::socket> _socketPtr;  // 套接字智能指针
        std::unique_ptr<std::thread> _threadPtr;  // 线程智能指针
        std::unique_ptr<Timer> _reconnectTimerPtr;  // 重连定时器智能指针
        boost::asio::ip::tcp::resolver::iterator _endpointIterator;

        char *_receive;  // 接收数据缓存
        SendQueue _sendQueue;  // 发送队列
        std::mutex _mutex;
    };

} /* namespace Dream */

#endif /* ASIOTCPCLIENT_H_ */
