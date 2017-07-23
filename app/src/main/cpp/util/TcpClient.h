/*
 * MsgTcpClient.h
 *
 *  Created on: 2016年9月30日
 *      Author: zhourui
 *      封装协议包以MsgPacket为基础的tcp客户端客户端
 *      派生类继承该类使用
 */

#ifndef TCPCLIENT_H_
#define TCPCLIENT_H_

#include "AsioTcpClient.h"
#include "BasePacket.h"

namespace Dream {

// 使用包继承basepacket
    template<typename Packet>
    class TcpClient {
    public:
        TcpClient() = delete;

        TcpClient(ThreadPool &pool, const std::string &address, unsigned port) :
                _tcpClient(
                        std::bind(&TcpClient::receiveData, this, std::placeholders::_1,
                                  std::placeholders::_2),
                        address, port), _currentStatus(AsioTcpClient::NO_CONNECT), _pool(
                pool), _isHeader(true), _readLength(Packet::HEADER_LENGTH),
                _connectStatusListener(nullptr) {
            // 设置状态侦听
            _tcpClient.setStatusListener(
                    std::bind(&TcpClient::onStatus, this, std::placeholders::_1));
            // 设置读取长度回调
            _tcpClient.setReadLengthCallback(std::bind(&TcpClient::readLenght, this));
        }

        virtual ~TcpClient() {

        }

        // 是否已连接
        bool isConnected() {
            return _currentStatus == AsioTcpClient::CONNECTED;
        }

        // 启动
        virtual bool startUp() {
            return _tcpClient.startUp();
        }

        // 关闭
        virtual void shutDown() {
            _tcpClient.shutDown();
            _tcpClient.removeCallback();
            _tcpClient.removeStatusListener();
            _tcpClient.removeReadLengthCallback();
        }

        // 发送包
        bool send(Packet &packet) {
            std::size_t packetLen = packet.computeLength();
            packet.length(packetLen);
            if (packetLen > 0) {
                auto dataPtr = std::make_shared<PacketData>(packetLen);
                int len = packet.encode(dataPtr->data(), dataPtr->length());
                if (len > 0) {
                    return _tcpClient.send(dataPtr);
                }
            }
            return false;
        }

        // 设置连接状态侦听器
        void setConnectStatusListener(std::function<void(bool isConnected)> connectStatusListener) {
            _connectStatusListener = connectStatusListener;
        }

        void cancelConnectStatusListener() {
            _connectStatusListener = nullptr;
        }

        // 派生类重写开始－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－

        // 子类用这个函数解析包,返回一个Packet智能指针,不需要delete，底层会自动删除内存
        virtual std::shared_ptr<Packet> newPacketPtr(unsigned msgType) = 0;

        // 子类继承这个用来接收包
        virtual void receive(const std::shared_ptr<Packet> &packetPtr) = 0;

        // 状态回调，底层会自动重连，派生类处理自己的逻辑即可
        // 连接成功
        virtual void onConnect() = 0;

        // 连接断开
        virtual void onClose() = 0;

        // 派生类重写结束－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－
    private:
        // 底层调用该函数来读取相应的长度
        unsigned readLenght() {
            return _readLength;
        }

        void receiveData(const char *pBuf, std::size_t dataLen) {
            bool callReceive = false;  // 是否调用接收函数

//            LOGW("TcpClient", "receiveData datalen=%d\n", dataLen);

            // 是读取头
            if (_isHeader) {
                // 通过头解析包的总长度
                unsigned packetLen = 0;
                int tmp = 0;
                DECODE_INT32(pBuf, packetLen, tmp);
                if (packetLen >= dataLen && packetLen <= _tcpClient.receiveSize()) {

                    _packetDataPtr.reset(new PacketData(packetLen));  // 创建包内存
                    memcpy(_packetDataPtr->data(), pBuf, dataLen);  // 写入头

                    unsigned bodyLen = packetLen - dataLen;
                    if (bodyLen > 0) {  // 包含包体
                        _isHeader = false;
                        _readLength = bodyLen;
                    } else {  // 不包含包体的话，就直接调用接收函数
                        callReceive = true;
                    }
                } else {
                    LOGE("TcpClient", "包头解析长度出错，长度＝[%d]\n", packetLen);
//                    _tcpClient.shutDown(true);  // 这函数是io线程自身调用，必须加true
                    return;
                }
            } else {  // 读取body
                if (dataLen != _readLength) {  // 读取到的长度和想要读取的长度不相等的话，就解析错误，直接关闭客户端
                    LOGE("TcpClient", "读取到的长度[%zd]和需要读取长度[%d]不相等，无法解析数据\n", dataLen, _readLength);
//                    _tcpClient.shutDown(true);
                    return;
                }
                memcpy(_packetDataPtr->data() + Packet::HEADER_LENGTH, pBuf, dataLen);  // 写入body
                callReceive = true;
            }

            // 需要调用接收函数，也就是已经保存了一个完整包的时候
            if (callReceive) {
                // 调用接收之后，继续接收头
                _isHeader = true;
                _readLength = Packet::HEADER_LENGTH;

                unsigned msgType = 0;
                int tmp = 0;
                DECODE_INT32(_packetDataPtr->data() + sizeof(int), msgType, tmp);
                // 向下转型
                auto packetPtr = newPacketPtr(msgType);

                if (packetPtr.get() != nullptr) {
                    int usedLen = packetPtr->decode(_packetDataPtr->data(),
                                                    _packetDataPtr->length());
                    // 解析完成后，需要减少当前线程的引用计数，使得该临时内存更快释放
                    _packetDataPtr.reset();
                    if (usedLen > 0) {
                        // 这里使用线程池提高并发处理能力
                        _pool.addTask(std::bind(&TcpClient::receive, this, packetPtr));
                    }
                }
            }

        }

        void onStatus(AsioTcpClient::Status status) {
            // 未连接变成已连接
            if (_currentStatus != AsioTcpClient::CONNECTED
                && status == AsioTcpClient::CONNECTED) {
                onConnect();
                if (_connectStatusListener) {
                    _connectStatusListener(true);
                }
            } else if (_currentStatus == AsioTcpClient::CONNECTED
                       && status != AsioTcpClient::CONNECTED) {
                // 已连接变成未连接
                onClose();
                if (_connectStatusListener) {
                    _connectStatusListener(false);
                }
            }
            _currentStatus = status;
        }

    private:
        AsioTcpClient _tcpClient;
        AsioTcpClient::Status _currentStatus;

        ThreadPool &_pool;
        bool _isHeader;  // 是否是头
        unsigned _readLength;  // 下次需要读取的长度，一般是先读头，解析出body长度后在读body，然后再读头，依次循环
        std::shared_ptr<PacketData> _packetDataPtr;  // 这个是用来拼包的

        std::function<void(bool isConnected)> _connectStatusListener;
        std::mutex _mutex;
    };

} /* namespace Dream */

#endif /* MSGTCPCLIENT_H_ */
