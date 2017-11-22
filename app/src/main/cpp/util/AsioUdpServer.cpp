/*
 * AsioUdpServer.cpp
 *
 *  Created on: 2016年9月27日
 *      Author: zhourui
 *      udp服务类，包含发送和接受功能
 */

#include <boost/asio.hpp>
#include "AsioUdpServer.h"
#include "public/platform.h"

#ifdef _ANDROID
extern JavaVM *g_pJavaVM;  // 定义外部变量，该变量在VmNet-lib.cpp中被定义和赋值
#endif

namespace Dream {

    AsioUdpServer::AsioUdpServer(
            std::function<void(const std::string &remote_address, unsigned short remote_port,
                               const char *pBuf, std::size_t len)> callback,
            const std::string &local_address, unsigned short local_port, bool ipv6,
            unsigned receiveSize, unsigned sendSize)
            : _callback(callback), _local_address(local_address), _local_port(local_port),
              _receiveSize(receiveSize), _ipv6(ipv6), _sendSize(sendSize), _receive(nullptr) {
        _receive = new char[receiveSize];
    }

    AsioUdpServer::~AsioUdpServer() {
        if (_receive != nullptr) {
            delete[] _receive;
            _receive = nullptr;
        }
    }

    bool AsioUdpServer::start_up(const std::string& multicast_addr) {
        std::lock_guard<std::mutex> lock(_mutex);
        // 初始化
        if (!do_init()) {
            LOGE("AsioUdpServer", "[%s]AsioUdpServer init failed!\n", _local_address.c_str());
            return false;
        }

        if (_socketPtr.get() && !multicast_addr.empty()) {
            boost::asio::ip::address multicast_address = boost::asio::ip::address::from_string(multicast_addr);
            boost::asio::ip::multicast::join_group option(multicast_address);
            _socketPtr->set_option(option);
        }

        do_read();

        _threadPtr.reset(new std::thread(&AsioUdpServer::run, this));

        return true;
    }

    void AsioUdpServer::shut_down() {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_ioServicePtr.get()) {
            _ioServicePtr->post([this]() {
                close();
            });
        }
        if (_threadPtr.get()) {
            if (_threadPtr->joinable()) {
                LOGW("AsioUdpServer", "AsioUdpServer [%s:%d] waiting ioService stop, join\n", _local_address.c_str(), _local_port);
                _threadPtr->join();
            } else {
                LOGW("AsioUdpServer", "AsioUdpServer [%s:%d] waiting ioService stop, detach\n", _local_address.c_str(), _local_port);
                _threadPtr->detach();
            }

            LOGW("AsioUdpServer", "AsioUdpServer [%s:%d] waited ioService stop...\n",
                 _local_address.c_str(), _local_port);

            _threadPtr.reset();
        }
        if (_ioServicePtr.get()) {
            _ioServicePtr.reset();
        }
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
        LOGW("AsioUdpServer", "[%s]AsioUdpServer initing...\n", _local_address.c_str());

        if (!_ioServicePtr.get()) {
            _ioServicePtr.reset(new boost::asio::io_service());
        }

        bool bindSuccess = false;
        while (!bindSuccess) {
            try {
                if (!_local_address.empty()) {
                    boost::asio::ip::address address;
                    if (_local_address.find(":") != std::string::npos) {
                        address = boost::asio::ip::address_v6::from_string(_local_address);
                    } else {
                        address = boost::asio::ip::address_v4::from_string(_local_address);
                    }
                    boost::asio::ip::udp::endpoint endpoint(address, _local_port);
                    if (_socketPtr.get() == nullptr) {
                        _socketPtr.reset(
                                new boost::asio::ip::udp::socket(*(_ioServicePtr.get()),
                                                                 endpoint));
                    }
                } else {
                    boost::asio::ip::udp::endpoint endpoint(
                            _ipv6 ? boost::asio::ip::udp::v6() : boost::asio::ip::udp::v4(),
                            _local_port);
                    if (_socketPtr.get() == nullptr) {
                        _socketPtr.reset(
                                new boost::asio::ip::udp::socket(*(_ioServicePtr.get()),
                                                                 endpoint));
                    }
                }
                
                if (_local_address.empty()) {
                    _local_address = local_address();
                }
                
                if (_local_port == 0) {
                    _local_port = local_port();
                }
                
                bindSuccess = true;
            } catch (...) {
                ++_local_port;
            }
        }

        LOGW("AsioUdpServer", "[%s] AsioUdpServer udp port[%d] binded success\n", _local_address.c_str(), _local_port);

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

        boost::asio::ip::address address;
        if (_ipv6) {
            address = boost::asio::ip::address_v6::from_string(_sendQueue.front()->address());
        } else {
            address = boost::asio::ip::address_v4::from_string(_sendQueue.front()->address());
        }
        _socketPtr->async_send_to(
                boost::asio::buffer(_sendQueue.front()->data(), _sendQueue.front()->length()),
                boost::asio::ip::udp::endpoint(address, _sendQueue.front()->port()),
                [this](boost::system::error_code ec, std::size_t /*length*/) {
                    if (!ec) {
                        _sendQueue.pop();
                    } else {
                        LOGE("AsioUdpServer", "AsioUdpServer send data error_code[%d] exception [%s]\n",
                             ec.value(), ec.message().c_str());
                    }
                    if (!_sendQueue.empty()) {
                        do_write();
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
                LOGW("AsioUdpServer", "[%s:%d] attach android thread success！\n",
                     _local_address.c_str(),
                     _local_port);
            } else {
                LOGE("AsioUdpServer", "[%s:%d] attach android thread failed！\n",
                     _local_address.c_str(),
                     _local_port);
                return;
            }
        } else {
            LOGW("AsioUdpServer", "[%s:%d]pJavaVm is null！\n", _local_address.c_str(), _local_port);
        }
#endif
        // 增加一个work对象
        boost::asio::io_service::work work(*(_ioServicePtr.get()));
        LOGW("AsioUdpServer", "io service start!\n");
        try {
            _ioServicePtr->run();
        } catch (...) {
            LOGE("AsioUdpServer", "_ioServicePtr running exception！！\n");
        }
        LOGW("AsioUdpServer", "io service stoped!\n");
#ifdef _ANDROID
        // 解绑android线程
        if (g_pJavaVM && pJniEnv) {
            LOGW("AsioUdpServer", "[%s][%d] detach android thread！\n", _local_address.c_str(),
                 _local_port);
            g_pJavaVM->DetachCurrentThread();
        }
#endif
    }

    void AsioUdpServer::close() {
// 这里一定要单独捕获异常，如果这里出错，下面没有执行的话，类析构时会崩溃，可能时由于线程还未结束，在线程析构前，必须要调用detach或者join
        if (_socketPtr.get()) {
            try {
//                _socketPtr->shutdown(boost::asio::ip::udp::socket::shutdown_both);
                _socketPtr->close();  // 关闭套接字
            } catch (std::exception &e) {
                LOGE("AsioUdpServer", "AsioUdpServer [%s]_socketPtr->close() exception！！[%s]\n",
                     _local_address.c_str(),
                     e.what());  // 一般是由于本机网络异常，导致endpoint未连接
            }

            _socketPtr.reset();
        }

        if (_ioServicePtr.get()) {
            LOGW("AsioUdpServer", "stop io service begin\n");
            _ioServicePtr->stop();
            LOGW("AsioUdpServer", "stop io service end\n");
//            _ioServicePtr.reset();
        }
    }


} /* namespace Dream */
