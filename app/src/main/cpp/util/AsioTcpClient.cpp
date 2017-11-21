/*
 * AsioTcpClient.cpp
 *
 *  Created on: 2016年9月27日
 *      Author: zhourui
 */

#include "AsioTcpClient.h"

#ifdef _ANDROID
extern JavaVM *g_pJavaVM;  // 定义外部变量，该变量在VmNet-lib.cpp中被定义和赋值
#endif

namespace Dream {

    AsioTcpClient::AsioTcpClient(
            std::function<void(const char *pBuf, std::size_t len)> callback,
            const std::string &address, unsigned short port,
            unsigned receiveSize, unsigned sendSize, unsigned reconnectInterval) :
            _callback(callback), _address(address), _port(port), _receiveSize(
            receiveSize), _sendSize(sendSize), _reconnectInterval(
            reconnectInterval), _currentStatus(NO_CONNECT), _statusListener(
            [](int status) -> void { printf("当前状态[%d]\n", status); }), _receive(
            nullptr), _ioServicePtr(nullptr), _socketPtr(nullptr), _threadPtr(nullptr),
            _reconnectTimerPtr(nullptr) {
        _receive = new char[receiveSize];
    }

    AsioTcpClient::~AsioTcpClient() {
        if (_receive != nullptr) {
            delete[] _receive;
        }
    }

    bool AsioTcpClient::startUp() {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_currentStatus.load() != NO_CONNECT && _currentStatus.load() != CLOSED) {
            return false;
        }

        // 初始化
        if (!doInit()) {
            LOGE("AsioTcpClient", "[%s:%d]初始化失败!\n", _address.c_str(), _port);
            _currentStatus.store(NO_CONNECT);
            return false;
        }

        // 发起连接
        doConnect();
//  _sendThreadPtr->detach();
//
//  std::thread thread(&AsioTcpClient::run, this);
//  thread.detach();

        _threadPtr.reset(new std::thread(&AsioTcpClient::run, this));

        return true;
    }

    void AsioTcpClient::shutDown(bool isIoServiceThread) {
        std::lock_guard<std::mutex> lock(_mutex);

        if (_ioServicePtr.get()) {
            _ioServicePtr->post([this, isIoServiceThread]() {
                close(isIoServiceThread);
            });
        }

        if (!isIoServiceThread && _threadPtr.get() && _threadPtr->joinable()) {
            LOGW("AsioTcpClient", "[%s:%d]开始等待ioService停止...\n", _address.c_str(), _port);
            _threadPtr->join();  // 等待线程执行完毕，避免线程还在连接的时候进行关闭操作
            LOGW("AsioTcpClient", "[%s:%d]结束等待ioService停止...\n", _address.c_str(), _port);
            _threadPtr.reset();
        }

        if (_ioServicePtr.get()) {
            _ioServicePtr.reset();
        }
    }

    bool AsioTcpClient::send(const std::shared_ptr<PacketData> &dataPtr) {
        std::lock_guard<std::mutex> lock(_mutex);
//        LOGW("AsioTcpClient", "[%s]开始发送数据...\n", _address.c_str());
        if (_currentStatus.load() != CONNECTED || _ioServicePtr.get() == nullptr) {
//    printf("当前处于未连接状态，无法发送数据\n");
            return false;
        }
        // post是异步操作，会投放到service的队列中，这里不需要加锁就是线程安全的
        _ioServicePtr->post([this, dataPtr]() {
            bool sending = !_sendQueue.empty();
            _sendQueue.emplace(dataPtr);
            if (!sending) {
                doWrite();
            }
        });

        return true;
    }

    bool AsioTcpClient::send(const char *buf, std::size_t len) {
        auto dataPtr = std::make_shared<PacketData>(len, buf);
        return send(dataPtr);
    }

    bool AsioTcpClient::sendSync(const std::shared_ptr<PacketData> &dataPtr) {
        std::lock_guard<std::mutex> lock(_mutex);
//        LOGW("AsioTcpClient", "[%s]开始发送数据...\n", _address.c_str());
        if (_currentStatus.load() != CONNECTED || _ioServicePtr.get() == nullptr) {
//    printf("当前处于未连接状态，无法发送数据\n");
            return false;
        }

        std::size_t sendSize = boost::asio::write(*(_socketPtr.get()),
                                                  boost::asio::buffer(dataPtr->data(),
                                                                      dataPtr->length()));
        return sendSize > 0;
    }

    bool AsioTcpClient::sendSync(const char *buf, std::size_t len) {
        auto dataPtr = std::make_shared<PacketData>(len, buf);
        return sendSync(dataPtr);
    }

    bool AsioTcpClient::doInit() {
        if (_currentStatus.load() == INITING) {
            return false;
        }

        if (_address.empty()) {
            LOGE("AsioTcpClient", "地址不能为[%s:%d]\n", _address.c_str(), _port);
        }
        LOGW("AsioTcpClient", "[%s:%d]正在初始化...\n", _address.c_str(), _port);
        _currentStatus.store(INITING);
        doOnStatus(_currentStatus);

        if (_ioServicePtr.get() == nullptr) {
            _ioServicePtr.reset(new boost::asio::io_service());
        }

        char port[10];
        snprintf(port, 10, "%d", _port);

        try {
            boost::asio::ip::tcp::resolver resolver(*(_ioServicePtr.get()));
            _endpointIterator = resolver.resolve({_address.c_str(), port});
        } catch (boost::system::system_error &e) {
            _ioServicePtr.reset(nullptr);
            _currentStatus.store(CLOSED);
            LOGE("AsioTcpClient", "网络异常，地址[%s:%d]，错误信息[%s]\n", _address.c_str(), _port, e.what());
            return false;
        }

        if (_socketPtr.get() == nullptr) {
            _socketPtr.reset(new boost::asio::ip::tcp::socket(*(_ioServicePtr.get())));
        }

        return true;
    }

    void AsioTcpClient::doConnect() {
        if (_currentStatus.load() == CONNECTING || _socketPtr.get() == nullptr) {
            LOGE("AsioTcpClient", "[%s:%d]取消连接...\n", _address.c_str(), _port);
            return;
        }

        LOGW("AsioTcpClient", "[%s:%d]开始连接...\n", _address.c_str(), _port);
        _currentStatus.store(CONNECTING);
        doOnStatus(_currentStatus);

//    LOGW("AsioTcpClient", "[%s]async_connect开始...\n", _address.c_str());
        boost::asio::async_connect(*(_socketPtr.get()), _endpointIterator,
                                   [this](boost::system::error_code ec,
                                          boost::asio::ip::tcp::resolver::iterator) {
                                       if (!ec) {
                                           LOGW("AsioTcpClient", "[%s:%d]连接成功...\n",
                                                _address.c_str(), _port);
                                           _currentStatus.store(CONNECTED);
                                           doOnStatus(_currentStatus);
                                           if (_readLengthCallback) {  // 如果上层设置了读取长度
                                               if (_ioServicePtr.get() != nullptr) {
                                                   _ioServicePtr->post([this] {
                                                       doReadByLength();
                                                   });
                                               }
                                           } else {
                                               if (_ioServicePtr.get() != nullptr) {
                                                   _ioServicePtr->post([this] {
                                                       doRead();
                                                   });
                                               }
                                           }
                                       } else {
                                           LOGE("AsioTcpClient", "[%s:%d]连接失败 [%s]\n",
                                                _address.c_str(), _port,
                                                ec.message().c_str());
                                           if (ec.value() != 125) {
                                               doReconnect();
                                           }
                                       }
                                   });

//    LOGW("AsioTcpClient", "[%s]async_connect结束...\n", _address.c_str());
    }

    void AsioTcpClient::doReconnect() {
        // 如果是正在关闭状态，那么就不需要重连
        if (_currentStatus.load() == CLOSEING || _currentStatus.load() == CLOSED) {
            return;
        }
        LOGW("AsioTcpClient", "[%s:%d]等待重连...\n", _address.c_str(), _port);
        _currentStatus.store(NO_CONNECT);
        doOnStatus(_currentStatus);

//    safeSleep(_reconnectInterval);

//    doConnect();

        if (_reconnectTimerPtr.get() != nullptr) {
//      LOGW("AsioTcpClient", "[%s]timer cancel start...\n", _address.c_str());
            _reconnectTimerPtr->cancel();
//      LOGW("AsioTcpClient", "[%s]timer cancel end...\n", _address.c_str());
        }
        _reconnectTimerPtr.reset(
                new Timer(std::bind(&AsioTcpClient::doConnect, this), _reconnectInterval,
                          false, 0));

        _reconnectTimerPtr->start();
    }

    void AsioTcpClient::doReadByLength() {
        if (_socketPtr.get() == nullptr) {
            return;
        }
        unsigned readLen = getReadLength();
        if (readLen == 0) {
            LOGE("AsioTcpClient", "[%s:%d]上层设置的读取长度不合法，长度[%d]，无法读取数据，强制断开客户端\n", _address.c_str(),
                 _port, readLen);
//            shutDown(true);
            return;
        }
        boost::asio::async_read(*(_socketPtr.get()),
                                boost::asio::buffer(_receive, readLen),
                                [this, readLen](boost::system::error_code ec,
                                                std::size_t /*length*/) {
                                    if (!ec) {
                                        // std::shared_ptr<PacketData> dataPtr = std::make_shared<PacketData>(readLen, _receive);
                                        // 原先考虑使用线程池，但是不利于拼包，改成同步
//                                        LOGW("AsioTcpClient", "接收到数据");
//                                        std::unique_lock<std::mutex> lock(_mutex);
                                        if (_callback) {
//                                            LOGW("AsioTcpClient", "回调数据");
                                            _callback(_receive, readLen);
                                        }
//                                        lock.unlock();
                                        if (_ioServicePtr.get() != nullptr) {
                                            _ioServicePtr->post([this] {
                                                doReadByLength();
                                            });
                                        }

                                    } else {
                                        // 异常的时候可以分为多种情况，测试出，当本机网络断开时，重连会导致崩溃
                                        LOGE("AsioTcpClient", "[%s:%d]读取数据异常, 连接断开:%s, error_code[%d]\n",
                                             _address.c_str(), _port, ec.message().c_str(), ec.value());
                                        if (ec.value() != 125) {
                                            doReconnect();
                                        }
                                    }
                                });
    }

    void AsioTcpClient::doRead() {
        if (_socketPtr.get() == nullptr) {
            return;
        }
        _socketPtr->async_read_some(boost::asio::buffer(_receive, _receiveSize),
                                    [this](boost::system::error_code ec, std::size_t length) {
                                        if (!ec && length > 0) {
                                            // std::shared_ptr<PacketData> dataPtr = std::make_shared<PacketData>(length, _receive);
                                            // 原先考虑使用线程池，但是不利于拼包，改成同步，让上层处理拼包
//                                            std::unique_lock<std::mutex> lock(_mutex);
                                            if (_callback) {
                                                _callback(_receive, length);
                                            }
//                                            lock.unlock();
                                            if (_ioServicePtr.get() != nullptr) {
                                                _ioServicePtr->post([this] {
                                                    doRead();
                                                });
                                            };
                                        } else if (ec) {
                                            LOGE("AsioTcpClient",
                                                 "[%s:%d]读取数据异常, 连接断开:%s, error_code[%d]\n",
                                                 _address.c_str(), _port, ec.message().c_str(), ec.value());
                                            if (ec.value() != 125) {
                                                doReconnect();
                                            }
                                        }
                                    });
    }

    void AsioTcpClient::doWrite() {
        if (_socketPtr.get() == nullptr) {
            return;
        }
        boost::asio::async_write(*(_socketPtr.get()),
                                 boost::asio::buffer(_sendQueue.front()->data(),
                                                     _sendQueue.front()->length()),
                                 [this](boost::system::error_code ec, std::size_t /*length*/) {
                                     if (!ec) {
                                         _sendQueue.pop();
                                         if (!_sendQueue.empty()) {
//            printf("发送中...\n");
                                             doWrite();
                                         }
                                     } else {
                                         LOGE("AsioTcpClient", "[%s:%d]发送数据异常 [%s] error_code[%d]\n",
                                              _address.c_str(), _port, ec.message().c_str(), ec.value());
                                         if (ec.value() != 125) {
                                             doReconnect();
                                         }
                                     }
                                 });
    }

    void AsioTcpClient::run() {
        if (_ioServicePtr.get() == nullptr) {
            return;
        }
#ifdef _ANDROID
        // 绑定android线程
        JNIEnv *pJniEnv = nullptr;
        if (g_pJavaVM) {
            if (g_pJavaVM->AttachCurrentThread(&pJniEnv, nullptr) == JNI_OK) {
                LOGW("AsioTcpClient", "[%s:%d]绑定android线程成功pJniEnv[%zd]！\n", _address.c_str(), _port,
                     pJniEnv);
            } else {
                LOGE("AsioTcpClient", "[%s:%d]绑定android线程失败！\n", _address.c_str(), _port);
                return;
            }
        } else {
            LOGE("AsioTcpClient", "[%s:%d]pJavaVm 为空！\n", _address.c_str(), _port);
        }
#endif
        // 增加一个work对象
        boost::asio::io_service::work work(*(_ioServicePtr.get()));
        try {
//            LOGE("AsioTcpClient", "_ioServicePtr run start！！\n");
            _ioServicePtr->run();
//            LOGE("AsioTcpClient", "_ioServicePtr run end！！\n");
        } catch (...) {
            LOGE("AsioTcpClient", "[%s:%d]_ioServicePtr 运行异常！！\n", _address.c_str(), _port);
        }
#ifdef _ANDROID
        // 解绑android线程
        if (g_pJavaVM && pJniEnv) {
            LOGW("AsioTcpClient", "[%s:%d]解绑android线程[%zd]！\n", _address.c_str(), _port, pJniEnv);
            g_pJavaVM->DetachCurrentThread();
        }
#endif
    }

    void AsioTcpClient::close(bool isIoServiceThread) {
//        if (_currentStatus.load() == CLOSEING || _currentStatus.load() == CLOSED) {
//            return;
//        }

        if (_reconnectTimerPtr.get()) {
            _reconnectTimerPtr->cancel();
            _reconnectTimerPtr.reset();
        }

        // 由于会在析构时调用，所以全部放进try中防止析构时出错导致的内存溢出
        try {
            _currentStatus.store(CLOSEING);
            if (isIoServiceThread) {  // 是io线程的操作才进行回调
                doOnStatus(_currentStatus);
            }

            // 这里一定要单独捕获异常，如果这里出错，下面没有执行的话，类析构时会崩溃，可能时由于线程还未结束，在线程析构前，必须要调用detach或者join
            if (_socketPtr.get()) {
                try {
                    _socketPtr->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
                    _socketPtr->close();  // 关闭套接字
                } catch (std::exception &e) {
                    LOGE("AsioTcpClient", "[%s:%d]_socketPtr->close()异常！！[%s]\n", _address.c_str(),
                         _port,
                         e.what());  // 一般是由于本机网络异常，导致endpoint未连接
                }

                _socketPtr.reset();
            }

            if (!isIoServiceThread && _ioServicePtr.get()) {
                _ioServicePtr->stop();
            }

            _currentStatus.store(CLOSED);
            if (isIoServiceThread) {
                doOnStatus(_currentStatus);
            }

            LOGW("AsioTcpClient", "[%s:%d]closed！\n", _address.c_str(),
                 _port);  // 一般是由于本机网络异常，导致endpoint未连接
        } catch (std::exception &e) {
            LOGE("AsioTcpClient", "[%s:%d]close异常！！[%s]\n", _address.c_str(), _port,
                 e.what());  // 一般是由于本机网络异常，导致endpoint未连接
        }
    }

} /* namespace Dream */
