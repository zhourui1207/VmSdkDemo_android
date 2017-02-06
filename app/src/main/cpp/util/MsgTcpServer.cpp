/*
 * MsgTcpServer.cpp
 *
 *  Created on: 2016年10月9日
 *      Author: zhourui
 */

#include "MsgTcpServer.h"

namespace Dream {

    void MsgTcpServer::send(unsigned clientId, MsgPacket &msgPacket) {
        std::size_t packetLen = msgPacket.computeLength();
        msgPacket.length(packetLen);
        if (packetLen > 0) {
            auto dataPtr = std::make_shared<PacketData>(packetLen);
            int len = msgPacket.encode(dataPtr->data(), dataPtr->length());
            if (len > 0) {
                _tcpServer.send(clientId, dataPtr);
            }
        }
    }

    void MsgTcpServer::sendAll(MsgPacket &msgPacket) {
        std::size_t packetLen = msgPacket.computeLength();
        if (packetLen > 0) {
            auto dataPtr = std::make_shared<PacketData>(packetLen);
            int len = msgPacket.encode(dataPtr->data(), dataPtr->length());
            if (len > 0) {
                _tcpServer.sendAll(dataPtr);
            }
        }
    }

    void MsgTcpServer::receiveData(unsigned clientId, std::shared_ptr<PacketData> &dataPtr) {
        printf("开始处理数据\n");
        // todo： 这里并没有对拼包的情况做处理，用到服务端时再实现
        int dataLen = dataPtr->length();
        int usedLen = 0;
        unsigned msgType = 0;
        while (dataLen > 0 && usedLen < dataLen) {
            int tmp = 0;
            DECODE_INT(dataPtr->data() + sizeof(int), msgType, tmp);
            auto packetPtr = newPacketPtr(msgType);

            if (packetPtr.get() == nullptr) {
                printf("空指针错误!\n");
                return;
            }

            usedLen = packetPtr->decode(dataPtr->data(), dataLen);
            if (usedLen > 0) {
                _pool.addTask(std::bind(&MsgTcpServer::receive, this, clientId, packetPtr));
                // receive(clientId, packetPtr);
                dataLen -= usedLen;
            } else {
                dataLen = -1;  // 跳出循环
            }
        }

        printf("处理完成\n");
    }

    void MsgTcpServer::onStatus(AsioTcpServer::Status status) {
        _currentStatus = status;
        printf("状态改变[%d]\n", _currentStatus);
    }

    void MsgTcpServer::onConnectStatus(unsigned clientId,
                                       const std::shared_ptr<TcpClientSession> &sessionPtr,
                                       bool isConnect) {
        if (isConnect) {
            onConnect(clientId, sessionPtr);
        } else {
            onClose(clientId, sessionPtr);
        }
    }

} /* namespace Dream */
