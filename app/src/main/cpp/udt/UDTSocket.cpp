//
// Created by zhou rui on 2017/7/13.
//

#include "UDTSocket.h"
#include "UDTMultiplexer.h"
#include "packet/UDTShutdownPacket.h"
#include "packet/UDTSynPacket.h"
#include "packet/UDTKeepAlivePacket.h"
#include "packet/UDTAckPacket.h"

namespace Dream {

    UDTSocket::UDTSocket(SocketType socketType, ConnectionType connectionType,
                         std::shared_ptr<UDTMultiplexer> multiplexePtr, unsigned socketId,
                         const std::string &remoteAddr, unsigned short remotePort,
                         fUDTDataCallBack dataCallBack, fUDTConnectStatusCallBack statusCallBack,
                         void *pUser, unsigned maxPacketSize, unsigned maxWindowSize)
            : _socketStatus(SHUTDOWN), _socketType(socketType), _connectionType(connectionType),
              _maxWindowSize(maxPacketSize), _maxPacketSize(maxWindowSize), _cookie(0),
              _multiplexerPtr(multiplexePtr), _socketId(socketId), _remoteAddr(remoteAddr),
              _remotePort(remotePort), _dataCallBack(dataCallBack), _statusCallBack(statusCallBack),
              _user(pUser), _running(false), _firstAck(true), _ackSeqNumber(0), _rttUs(INIT_RTT),
              _rttVariance(0), _bufferSize(INIT_BUFFER_SIZE), _receiveRate(0), _linkCapacity(0) {
        memset(_addrBytes, 0, 4);
        if (!remoteAddr.empty()) {
            unsigned ip = 0;
            if (ipv4_2int(remoteAddr, ip)) {
                _addrBytes[3] = ip;
            }
        }
    }

    void UDTSocket::setup() {
        if (_threadPtr.get() == nullptr) {
            _running = true;
            _threadPtr.reset(new std::thread(&UDTSocket::run, this));
        }
    }

    void UDTSocket::shutdownWithoutRemove() {
        if (_threadPtr.get() != nullptr) {
            _running = false;
            try {
                _threadPtr->join();
            } catch (...) {

            }
            _threadPtr.reset();
        }
    }

    void UDTSocket::shutdown() {
        std::unique_lock<std::mutex> lock(_mutex);

        shutdownWithoutRemove();

        // 从复用器中删除
        if (_multiplexerPtr.get() != nullptr) {
            _multiplexerPtr->removeSocket(_socketId);
        }
    }

    void UDTSocket::sendSynPacket() {
        if (_multiplexerPtr.get() != nullptr) {
            _seqNumber = rangeRand(0, UDTBasePacket::SEQUENCE_NUMBER_MAX);
            UDTSynPacket packet(UDT_VERSION, _socketType, _seqNumber, _maxPacketSize,
                                _maxWindowSize, _connectionType, _socketId, _cookie, _addrBytes);

            char tmpBuf[_maxPacketSize];
            int len = packet.encode(tmpBuf, _maxPacketSize);
            if (len > 0) {
                _multiplexerPtr->sendData(_remoteAddr, _remotePort, tmpBuf, (size_t) len);
            }
        }
    }

    void UDTSocket::sendKeepAlivePacket() {
        if (_multiplexerPtr.get() != nullptr) {
            UDTKeepAlivePacket packet;

            char tmpBuf[_maxPacketSize];
            int len = packet.encode(tmpBuf, _maxPacketSize);
            if (len > 0) {
                _multiplexerPtr->sendData(_remoteAddr, _remotePort, tmpBuf, (size_t) len);
            }
        }
    }

    void UDTSocket::sendAckPacket(unsigned seqNumber) {
        if (_multiplexerPtr.get() != nullptr) {

            if (_firstAck) {
                _ackSeqNumber = rangeRand(0, UDTControlPacket::ADDITIONAL_INFO_MAX);
                _firstAck = false;
            } else {
                if (_ackSeqNumber == UDTControlPacket::ADDITIONAL_INFO_MAX) {
                    _ackSeqNumber = 0;
                } else {
                    ++_ackSeqNumber;
                }
            }

            UDTAckPacket packet(_ackSeqNumber, seqNumber, _rttUs, _rttVariance, _bufferSize,
                                _receiveRate, _linkCapacity);

            char tmpBuf[_maxPacketSize];
            int len = packet.encode(tmpBuf, _maxPacketSize);
            if (len > 0) {
                _multiplexerPtr->sendData(_remoteAddr, _remotePort, tmpBuf, (size_t) len);
            }
        }
    }

    void UDTSocket::sendNakPacket() {

    }

    void UDTSocket::sendAck2Packet() {

    }

    // 发送停止包
    void UDTSocket::sendShutdownPacket() {
        if (_multiplexerPtr.get() != nullptr) {
            UDTShutdownPacket packet;
            char tmpBuf[_maxPacketSize];
            int len = packet.encode(tmpBuf, _maxPacketSize);
            if (len > 0) {
                _multiplexerPtr->sendData(_remoteAddr, _remotePort, tmpBuf, (size_t) len);
            }
        }
    }

    void UDTSocket::sendMdrPacket() {

    }

    void UDTSocket::receiveData(const std::string &remote_address, unsigned short remote_port,
                                const char *pBuf, std::size_t len) {

    }

    void UDTSocket::changeSocketStatus(SocketStatus socketStatus) {
        if (_socketStatus != socketStatus) {
            bool needCallback = false;
            if (_socketStatus == CONNECTED || socketStatus == CONNECTED) {
                needCallback = true;
            }
            _socketStatus = socketStatus;
            if (needCallback && _statusCallBack != nullptr) {
                _statusCallBack(_socketId, socketStatus == CONNECTED, _user);
            }
        }
    }

    void UDTSocket::run() {
        if (_socketStatus == SHUTDOWN) {
            if (_connectionType == REGULAR) {  // 监听模式

            } else {
                changeSocketStatus(CONNECTING);
            }
        } else {
            LOGE(TAG, "Syn must be called on shutdown status!\n");
        }

        while (_running) {

        }

        // 发送shutdown包
        if (_socketStatus != SHUTDOWN) {
            sendShutdownPacket();
        }

        //

    }
}