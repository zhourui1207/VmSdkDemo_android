//
// Created by zhou rui on 2017/7/13.
//

#include "UDTSocket.h"
#include "UDTMultiplexer.h"
#include "packet/UDTShutdownPacket.h"
#include "packet/UDTKeepAlivePacket.h"
#include "packet/UDTAckPacket.h"
#include "packet/UDTNakPacket.h"
#include "packet/UDTAck2Packet.h"

namespace Dream {

    UDTSocket::UDTSocket(SocketType socketType, ConnectionType connectionType,
                         std::shared_ptr<UDTMultiplexer> multiplexePtr, uint32_t socketId,
                         const std::string &remoteAddr, uint16_t remotePort,
                         fUDTDataCallBack dataCallBack, fUDTConnectStatusCallBack statusCallBack,
                         void *pUser, uint32_t maxPacketSize, uint32_t maxWindowSize)
            : _socketStatus(SHUTDOWN), _socketType(socketType), _connectionType(connectionType),
              _seqNumber(-1), _maxWindowSize(maxPacketSize), _maxPacketSize(maxWindowSize),
              _cookie(0), _multiplexerPtr(multiplexePtr), _socketId(socketId),
              _remoteAddr(remoteAddr), _remotePort(remotePort), _dataCallBack(dataCallBack),
              _statusCallBack(statusCallBack), _user(pUser), _running(false), _firstAck(true),
              _ackSeqNumber(-1), _rttUs(INIT_RTT_US), _rttVariance(0),
              _bufferSize(INIT_BUFFER_SIZE), _receiveRate(0), _linkCapacity(0), _rttSampleIndex(0),
              _stp(SYN_TIME_US), _lossedSize(0), _lrsn(-1), _timeoutNumber(0), _ackTime(0),
              _nakTime(0), _expTime(0), _sndTime(0), _handlePacket(false), _handling(false) {

        memset(_addrBytes, 0, 4);
        if (!remoteAddr.empty()) {
            uint32_t ip = 0;
            if (ipv4_2int(remoteAddr, ip)) {
                _addrBytes[3] = ip;
            }
        }

        memset(_rttSamples, _rttUs, RTT_SAMPLE_NUM);
    }

    UDTSocket::~UDTSocket() {

    }

    // 启动
    void UDTSocket::setup() {
        _handlePacket = true;

        if (_threadPtr.get() == nullptr) {
            _running = true;
            _threadPtr.reset(new std::thread(&UDTSocket::run, this));
        }
    }

    void UDTSocket::shutdownWithoutRemove() {
        _handlePacket = false;

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

    void UDTSocket::handleRevice() {

    }

    void UDTSocket::ackTask() {

    }

    void UDTSocket::nakTask() {

    }

    void UDTSocket::expTask() {

    }

    void UDTSocket::sndTask() {

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

    void UDTSocket::handleSynPacket(const UDTSynPacket &packet) {
        if (_socketStatus == SHUTDOWN) {
            if (_socketType == RENDEZVOUS) {
                int32_t connectionType = packet.connectionType();
                switch (connectionType) {
                    case 0:
                        break;
                    case -1:
                        break;
                    case -2:
                        break;
                    default:
                        // nothing
                        break;
                }
            } else {

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
        if (_multiplexerPtr.get() != nullptr) {

            UDTNakPacket packet(_lossedSeqNumber, _lossedSize);

            char tmpBuf[_maxPacketSize];
            int len = packet.encode(tmpBuf, _maxPacketSize);
            if (len > 0) {
                _multiplexerPtr->sendData(_remoteAddr, _remotePort, tmpBuf, (size_t) len);
            }
        }
    }

    void UDTSocket::sendAck2Packet(unsigned ackSeqNumber) {
        if (_multiplexerPtr.get() != nullptr) {

            UDTAck2Packet packet(ackSeqNumber);

            char tmpBuf[_maxPacketSize];
            int len = packet.encode(tmpBuf, _maxPacketSize);
            if (len > 0) {
                _multiplexerPtr->sendData(_remoteAddr, _remotePort, tmpBuf, (size_t) len);
            }
        }
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

    void UDTSocket::sendMDRPacket() {
        // todo:暂时没想好怎么写
    }

    void UDTSocket::receiveData(const std::string &remote_address, uint16_t remote_port,
                                const char *pBuf, std::size_t len) {
        if (_handlePacket) {
            auto udpPtr = std::make_shared<UdpData>(remote_address, remote_port, len, pBuf);
            Task task = std::bind(&UDTSocket::handleData, this, udpPtr);
            _threadPool.addTask(task);
        }
    }

    void UDTSocket::handleData(std::shared_ptr<UdpData> udpDataPtr) {
        const char *pBuf = udpDataPtr->data();
        std::size_t len = udpDataPtr->length();

        // 接收到包，先判断数据还是控制包
        if (len > 0) {
            if (UDTBasePacket::decodeControlStatic((const unsigned char) *pBuf)) {  // 控制包
                UDTControlPacket::ControlPacketType controlPacketType;
                if (UDTControlPacket::decodeControlTypeStatic(pBuf, len, controlPacketType)) {
                    if (controlPacketType == UDTControlPacket::ControlPacketType::SYN) {
                        UDTSynPacket synPacket;
                        synPacket.decode(pBuf, len);
                        handleSynPacket(synPacket);
                    } else if (controlPacketType ==
                               UDTControlPacket::ControlPacketType::KEEP_ALIVE) {

                    } else {
                        LOGE(TAG, "The control type [%d] is unsupported!\n", controlPacketType);
                    }
                }
            } else {

            }
        }
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
        LOGW(TAG, "Socket thread running!");

        while (_running) {
            checkSocketStatus();
            handleRevice();
            ackTask();
            nakTask();
            expTask();
            sndTask();
        }

        // 发送shutdown包
        if (_socketStatus != SHUTDOWN) {
            sendShutdownPacket();
        }

        //
        LOGW(TAG, "Socket thread stoped!");
    }

    void UDTSocket::checkSocketStatus() {
        if (_socketStatus == SHUTDOWN) {
            if (_connectionType == REGULAR) {  // 监听模式

            } else {
                changeSocketStatus(CONNECTING);
            }
        } else {
            LOGE(TAG, "Syn must be called on shutdown status!\n");
        }
    }
}