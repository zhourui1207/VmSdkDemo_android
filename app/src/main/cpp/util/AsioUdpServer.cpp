/*
 * AsioUdpServer.cpp
 *
 *  Created on: 2016年9月27日
 *      Author: zhourui
 *      udp服务类，包含发送和接受功能
 */

#include "AsioUdpServer.h"
#include "public/platform.h"

#ifdef _ANDROID
extern JavaVM *g_pJavaVM;  // 定义外部变量，该变量在VmNet-lib.cpp中被定义和赋值
#endif

namespace Dream {

    AsioUdpServer::AsioUdpServer(
            std::function<void(const std::string &remote_address, unsigned short remote_port,
                               const char *pBuf, std::size_t len)> callback,
            const std::string &local_address, unsigned short local_port = 0,
            unsigned receiveSize, unsigned sendSize)
            : _callback(callback), _local_address(local_address), _local_port(local_port),
              _receiveSize(receiveSize), _sendSize(sendSize), _receive(nullptr) {
        _receive = new char[receiveSize];
    }

    AsioUdpServer::~AsioUdpServer() {
        if (_receive != nullptr) {
            delete[] _receive;
            _receive = nullptr;
        }
    }

    bool AsioUdpServer::start_up() {
        std::lock_guard<std::mutex> lock(_mutex);
        // 初始化
        if (!do_init()) {
            LOGE("UdpTcpServer", "[%s]初始化失败!\n", _local_address.c_str());
            return false;
        }

        do_read();

        _threadPtr.reset(new std::thread(&AsioUdpServer::run, this));

        return true;
    }

    void AsioUdpServer::shut_down() {
        std::lock_guard<std::mutex> lock(_mutex);
        close();
    }

    bool AsioUdpServer::send(const std::shared_ptr<UdpData> &dataPtr) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_ioServicePtr.get() == nullptr) {
            return false;
        }
        _ioServicePtr->post([this, dataPtr]() {
            bool sending = !_sendQueue.empty();
            _sendQueue.emplace(dataPtr);
            if (!sending) {
                do_write();
            }
        });
        return true;
    }

    bool
    AsioUdpServer::send(const std::string &remoteAddr, unsigned short remotePort, const char *buf,
                        std::size_t len) {
        auto dataPtr = std::make_shared<UdpData>(remoteAddr, remotePort, len, buf);
        return send(dataPtr);
    }

    bool AsioUdpServer::do_init() {
        LOGW("AsioUdpServer", "[%s]正在初始化...\n", _local_address.c_str());

        if (_ioServicePtr.get() == nullptr) {
            _ioServicePtr.reset(new boost::asio::io_service());
        }

        bool bindSuccess = false;
        while (!bindSuccess) {
            try {
                if (_local_address.length() > 0) {
                    boost::asio::ip::udp::endpoint endpoint(
                            boost::asio::ip::address_v4::from_string(_local_address), _local_port);
                    if (_socketPtr.get() == nullptr) {
                        _socketPtr.reset(
                                new boost::asio::ip::udp::socket(*(_ioServicePtr.get()), endpoint));
                    }
                } else {
                    boost::asio::ip::udp::endpoint endpoint(boost::asio::ip::udp::v4(),
                                                            _local_port);
                    if (_socketPtr.get() == nullptr) {
                        _socketPtr.reset(
                                new boost::asio::ip::udp::socket(*(_ioServicePtr.get()), endpoint));
                    }
                }
                bindSuccess = true;
            } catch (...) {
                ++_local_port;
            }
        }

        LOGW("AsioUdpServer", "udp端口[%d]绑定成功\n", _local_port);

        return true;
    }

    void AsioUdpServer::do_read() {
        if (_socketPtr.get() == nullptr) {
            return;
        }

        _socketPtr->async_receive_from(boost::asio::buffer(_receive, _receiveSize),
                                       _remote_endpoint,
                                       [this](boost::system::error_code ec,
                                              std::size_t length) {
                                           if (!ec && length > 0) {
                                               if (_callback) {
                                                   _callback(_remote_endpoint.address().to_string(),
                                                             _remote_endpoint.port(), _receive,
                                                             length);

                                               }
                                           }
                                           do_read();
                                       });
    }

    void AsioUdpServer::do_write() {
        if (_socketPtr.get() == nullptr) {
            return;
        }
        _socketPtr->async_send_to(
                boost::asio::buffer(_sendQueue.front()->data(), _sendQueue.front()->length()),
                boost::asio::ip::udp::endpoint(
                        boost::asio::ip::address_v4::from_string(_sendQueue.front()->address()),
                        _sendQueue.front()->port()),
                [this](boost::system::error_code ec, std::size_t /*length*/) {
                    if (!ec) {
                        _sendQueue.pop();
                        if (!_sendQueue.empty()) {
                            do_write();
                        }
                    } else {
                        LOGE("AsioUdpServer", "发送数据异常 [%s]\n", ec.message().c_str());
                    }
                });
    }

    void AsioUdpServer::run() {
        if (_ioServicePtr.get() == nullptr) {
            return;
        }
#ifdef _ANDROID
        // 绑定android线程
        JNIEnv *pJniEnv = nullptr;
        if (g_pJavaVM) {
            if (g_pJavaVM->AttachCurrentThread(&pJniEnv, nullptr) == JNI_OK) {
                LOGW("AsioUdpServer", "[%s][%d]绑定android线程成功！\n", _local_address.c_str(),
                     _local_port);
            } else {
                LOGE("AsioUdpServer", "[%s][%d]绑定android线程失败！\n", _local_address.c_str(),
                     _local_port);
                return;
            }
        } else {
            LOGE("AsioUdpServer", "[%s]pJavaVm 为空！\n", _local_address.c_str());
        }
#endif
        // 增加一个work对象
        boost::asio::io_service::work work(*(_ioServicePtr));
        try {
            _ioServicePtr->run();
        } catch (...) {
            LOGE("AsioUdpServer", "_ioServicePtr 运行异常！！\n");
        }
#ifdef _ANDROID
        // 解绑android线程
        if (g_pJavaVM && pJniEnv) {
            LOGW("AsioUdpServer", "[%s][%d]解绑android线程！\n", _local_address.c_str(), _local_port);
            g_pJavaVM->DetachCurrentThread();
        }
#endif
    }

    void AsioUdpServer::close() {
// 这里一定要单独捕获异常，如果这里出错，下面没有执行的话，类析构时会崩溃，可能时由于线程还未结束，在线程析构前，必须要调用detach或者join
        if (_socketPtr.get() != nullptr) {
            try {
//                _socketPtr->shutdown(boost::asio::ip::udp::socket::shutdown_both);
                _socketPtr->close();  // 关闭套接字
            } catch (std::exception &e) {
                LOGE("AsioUdpServer", "[%s]_socketPtr->close()异常！！[%s]\n", _local_address.c_str(),
                     e.what());  // 一般是由于本机网络异常，导致endpoint未连接
            }

            _socketPtr.reset();
        }

        if (_ioServicePtr.get() != nullptr) {
            _ioServicePtr->stop();
        }

        if (_threadPtr.get() != nullptr) {
            LOGW("AsioUdpServer", "[%s]开始等待ioService停止...\n", _local_address.c_str());
            _threadPtr->join();  // 等待线程执行完毕，避免线程还在连接的时候进行关闭操作
            LOGW("AsioUdpServer", "[%s]结束等待ioService停止...\n", _local_address.c_str());

            _threadPtr.reset();
        }

        _ioServicePtr.reset();
    }


} /* namespace Dream */
