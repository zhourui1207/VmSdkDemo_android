/*
 * AsioUdpServer.h
 *
 *  Created on: 2016年9月27日
 *      Author: zhourui
 */

#ifndef ASIOUDPSERVER_H_
#define ASIOUDPSERVER_H_

#include <mutex>
#include <string>
#include <functional>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/udp.hpp>
#include <thread>
#include <queue>
#include "PacketData.h"
#include "UdpData.h"
#include "Noncopyable.h"

namespace Dream {

    class AsioUdpServer : public Noncopyable {
        using SendQueue = std::queue<std::shared_ptr<UdpData>>;

    private:
        static const unsigned DEFAULT_READ_SIZE = 100 * 1024;  // 接收时的默认缓存大小
        static const unsigned DEFAULT_WRITE_SIZE = 100 * 1024;  // 发送时的默认缓存大小

    public:
        AsioUdpServer() = delete;

        AsioUdpServer(
                std::function<void(const std::string &remote_address, unsigned short remote_port,
                                   const char *pBuf, std::size_t len)> callback,
                const std::string &local_address, unsigned short local_port,
                unsigned receiveSize = DEFAULT_READ_SIZE,
                unsigned sendSize = DEFAULT_WRITE_SIZE);

        virtual ~AsioUdpServer();

    public:
        bool start_up();

        void shut_down();

        bool send(const std::shared_ptr<UdpData> &dataPtr);

        bool send(const std::string& remoteAddr, unsigned short remotePort, const char* buf, std::size_t len);

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

    private:
        bool do_init();

        void do_read();

        void do_write();

        void run();

        void close();

    private:
        std::function<void(const std::string &remote_address, unsigned short remote_port,
                           const char *pBuf, std::size_t len)> _callback;

        std::string _local_address;
        unsigned short _local_port;
        unsigned _receiveSize;
        unsigned _sendSize;

        boost::asio::ip::udp::endpoint _remote_endpoint;

        // boost::asio::io_service _ioService;  // 异步网络服务
        // 如果要实现关闭后能在再启动运行，必须重新新建ioservice，所以这里使用指针
        std::unique_ptr<boost::asio::io_service> _ioServicePtr;
        std::unique_ptr<boost::asio::ip::udp::socket> _socketPtr;  // 套接字智能指针
        std::unique_ptr<std::thread> _threadPtr;  // 线程智能指针

        char *_receive;  // 接收数据缓存
        SendQueue _sendQueue;  // 发送队列
        std::mutex _mutex;
    };

} /* namespace Dream */

#endif /* ASIOUDPSERVER_H_ */
