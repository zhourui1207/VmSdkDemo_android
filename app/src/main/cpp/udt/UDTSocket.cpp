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
                         void *pUser, uint32_t maxPacketSize, uint32_t maxWindowSize,
                         std::size_t sndBufSize, std::size_t recBufSize)
            : _socketStatus(SHUTDOWN), _socketType(socketType), _connectionType(connectionType),
              _seqNumber(-1), _maxWindowSize(maxPacketSize), _maxPacketSize(maxWindowSize),
              _cookie(0), _multiplexerPtr(multiplexePtr), _socketId(socketId),
              _remoteAddr(remoteAddr), _remotePort(remotePort), _dataCallBack(dataCallBack),
              _statusCallBack(statusCallBack), _user(pUser), _running(false),
              _sndBufSize(sndBufSize), _sendGlideWindow(_maxWindowSize), _recBufSize(recBufSize),
              _pktIndex(-1), _packetPairIndex(-1), _receiveGlideWindow(nullptr, _maxWindowSize),
              _firstAck(true),
              _ackSeqNumber(-1), _rttUs(INIT_RTT_US), _rttVariance(0),
              _bufferSize(INIT_BUFFER_SIZE), _receiveRate(0), _linkCapacity(0), _rttSampleIndex(0),
              _snd(SYN_TIME_US), _lrsn(-1), _expCount(0), _ackTime(0),
              _nakTime(0), _expTime(0), _sndTime(0), _handlePacket(false), _handling(false),
              _setupTime(0) {

        memset(_addrBytes, 0, 4);
        if (!remoteAddr.empty()) {
            uint32_t ip = 0;
            if (ipv4_2int(remoteAddr, ip)) {
                _addrBytes[3] = ip;
            }
        }

        memset(_rttSamples, _rttUs, RTT_SAMPLE_NUM);
        memset(_pkt, 0, HISTORY_WINDOW_SIZE);
        memset(_packetPair, 0, HISTORY_WINDOW_SIZE);
    }

    UDTSocket::~UDTSocket() {

    }

    bool UDTSocket::postData(const char *pBuf, std::size_t len) {

        if (_sndBuf.size() >= _sndBufSize) {
            return false;
        }

        std::size_t dataLen = _maxPacketSize - UDTDataPacket::headerLength();

        std::unique_lock<std::mutex> lock(_sndBufMutex);
        if (_socketType == STREAM) {  // 流 分割成定长的包
            int position = 0;
            while (position < len) {
                std::size_t copyLen = (position + dataLen < len) ? dataLen : (len - position);
                _seqNumber = GlideWindow::increaseSeqNumber(_seqNumber);
                auto packetPtr = std::make_shared<UDTDataPacket>(pBuf + position, copyLen,
                                                                 _seqNumber);
                _sndBuf.emplace(packetPtr);
                position += dataLen;
            }
        } else {  // 报文
            if (len <= dataLen) {  // 小于最大长度，用only
                _seqNumber = GlideWindow::increaseSeqNumber(_seqNumber);
                auto packetPtr = std::make_shared<UDTDataPacket>(pBuf, len, _seqNumber);
                _sndBuf.emplace(packetPtr);
            } else {  // 大于最大包长
                bool first = true;
                int position = 0;
                while (position < len) {
                    bool end = (position + dataLen >= len);
                    std::size_t copyLen = end ? (len - position) : dataLen;
                    _seqNumber = GlideWindow::increaseSeqNumber(_seqNumber);
                    UDTDataPacket::DataPacketType dataPacketType = UDTDataPacket::DataPacketType::MIDDLE;
                    if (first) {
                        dataPacketType = UDTDataPacket::DataPacketType::START;
                        first = false;
                    } else if (end) {
                        dataPacketType = UDTDataPacket::DataPacketType::STOP;
                    }
                    auto packetPtr = std::make_shared<UDTDataPacket>(pBuf + position, copyLen,
                                                                     _seqNumber, dataPacketType);
                    _sndBuf.emplace(packetPtr);
                    position += dataLen;
                }
            }
        }
        return true;
    }

    // 启动
    void UDTSocket::setup() {
        // 启动是由复用器调用，无需加锁
        _handlePacket = true;
        _running = true;
        // 开始发送线程和接收线程
        if (_receiveThreadPtr.get() == nullptr) {
            _receiveThreadPtr.reset(new std::thread(&UDTSocket::receiveLoop, this));
        }

        if (_sendThreadPtr.get() == nullptr) {
            _sendThreadPtr.reset(new std::thread(&UDTSocket::sendLoop, this));
        }
    }

    void UDTSocket::shutdownWithoutRemove() {
        _handlePacket = false;

        _running = false;
        if (_sendThreadPtr.get() != nullptr) {
            try {
                _sendThreadPtr->join();
            } catch (...) {

            }
            _sendThreadPtr.reset();
        }

        if (_receiveThreadPtr.get() != nullptr) {
            try {
                _receiveThreadPtr->join();
            } catch (...) {

            }
            _receiveThreadPtr.reset();
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

    std::string UDTSocket::getRemoteAddr() const {
        return _remoteAddr;
    }

    unsigned short UDTSocket::getRemotePort() const {
        return _remotePort;
    }

    bool UDTSocket::handleRevice() {
        return false;
    }

    void UDTSocket::ackTask() {

    }

    void UDTSocket::nakTask() {

    }

    void UDTSocket::expTask() {

    }

    void UDTSocket::sndTask() {

    }

    inline void UDTSocket::updateRttVariance() {  // 获取rtt样本方差
        uint32_t sum = 0;
        for (int i = 0; i < RTT_SAMPLE_NUM; ++i) {
            uint32_t offset = _rttSamples[i] - _rttUs;
            sum += (offset * offset);
        }
        _rttVariance = sum / RTT_SAMPLE_NUM;
    }

    inline uint32_t UDTSocket::getNakPeriod() {
        return 4 * _rttUs + _rttVariance + SYN_TIME_US;
    }

    inline uint32_t UDTSocket::getExpPeriod() {
        unsigned period = _expCount * getNakPeriod();
        return period < EXP_PERIOD_MIN ? EXP_PERIOD_MIN : period;
    }

    inline bool UDTSocket::ackTimeout() {
        uint64_t currentTime = getCurrentTimeStampMicro();
        if (_ackTime > 0 && (_ackTime + SYN_TIME_US > currentTime)) {  // 没超时
            return false;
        } else {
            _ackTime = currentTime;
            return true;
        }
    }

    inline bool UDTSocket::nakTimeout() {
        uint64_t currentTime = getCurrentTimeStampMicro();
        if (_nakTime > 0 && (_nakTime + getNakPeriod() > currentTime)) {  // 没超时
            return false;
        } else {
            _nakTime = currentTime;
            return true;
        }
    }

    inline bool UDTSocket::expTimeout() {
        uint64_t currentTime = getCurrentTimeStampMicro();
        if (_expTime > 0 && (_expTime + getExpPeriod() > currentTime)) {  // 没超时
            return false;
        } else {
            _expTime = currentTime;
            return true;
        }
    }

    inline bool UDTSocket::sndTimeout() {
        uint64_t currentTime = getCurrentTimeStampMicro();
        if (_sndTime > 0 && (_sndTime + _snd > currentTime)) {  // 没超时
            return false;
        } else {
            _sndTime = currentTime;
            return true;
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

    void UDTSocket::handleSynPacket(const UDTSynPacket &packet) {
        if (_socketStatus == SHUTDOWN) {
            if (_connectionType == RENDEZVOUS) {
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

    void UDTSocket::sendAckPacket(int32_t seqNumber) {
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

    void UDTSocket::sendNakPacket(const std::vector<int32_t> &seqNumberList) {
        if (_multiplexerPtr.get() != nullptr) {

            std::size_t size = seqNumberList.size();
            int32_t *tmpArray = new int32_t(size);
            std::size_t arrayPos = 0;
            int32_t preSeqNmber = -1;
            for (std::size_t i = 0; i < size; ++i) {
                int32_t eleSeqNumber = seqNumberList[i];
                if (preSeqNmber >= 0 && arrayPos > 0 &&
                    GlideWindow::continuousSeqNumber(preSeqNmber, eleSeqNumber)) {  // 是连续的序列号
                    // 将前面的最高位置为1
                    tmpArray[arrayPos - 1] |= (0x80 << 24);
                } else {
                    tmpArray[arrayPos++] = eleSeqNumber;
                    preSeqNmber = eleSeqNumber;
                }
            }

            UDTNakPacket packet(tmpArray, arrayPos);
            char tmpBuf[_maxPacketSize];
            int len = packet.encode(tmpBuf, _maxPacketSize);
            if (len > 0) {
                _multiplexerPtr->sendData(_remoteAddr, _remotePort, tmpBuf, (size_t) len);
            }
        }
    }

    void UDTSocket::sendAck2Packet(int32_t ackSeqNumber) {
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
            if (_recBuf.size() < _recBufSize) {
                std::unique_lock<std::mutex> lock(_recBufMutex);
                auto udpPtr = std::make_shared<UdpData>(remote_address, remote_port, len, pBuf);
                _recBuf.emplace(udpPtr);
            }
        }
    }

    std::shared_ptr<UdpData> UDTSocket::tryGetReceivedPacket() {
        std::unique_lock<std::mutex> lock(_recBufMutex);

        std::shared_ptr<UdpData> udpDataPtr;
        if (!_recBuf.empty()) {
            udpDataPtr = _recBuf.front();
            _recBuf.pop();
        }
        return udpDataPtr;
    }

    void UDTSocket::handleControlPacket(std::shared_ptr<UdpData> udpDataPtr,
                                        const UDTControlPacket::ControlPacketType &controlPacketType) {
        const char *pBuf = udpDataPtr->data();
        std::size_t len = udpDataPtr->length();

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

    void UDTSocket::handleDataPacket(std::shared_ptr<UdpData> udpDataPtr) {
        auto dataPacketPtr = std::make_shared<UDTDataPacket>();
        if (dataPacketPtr->decode(udpDataPtr->data(), udpDataPtr->length()) > 0) {
            // 4.如果当前包的序列号是 16n+1，记录这个包跟上次最后一个数据包的时间间隔，存入包对窗口
            int32_t seqNumber = dataPacketPtr->seqNumber();
            uint64_t arrivalTime = getCurrentTimeStampMicro();
            if (_pktIndex >= 0 && seqNumber % 16 == 1) {
                int64_t timeInterval = arrivalTime - _pkt[_pktIndex];
                if (timeInterval < 0) {
                    timeInterval = 0;
                }
                if (_packetPairIndex < 0) {
                    _packetPairIndex = 0;
                } else if (++_packetPairIndex >= HISTORY_WINDOW_SIZE) {
                    _packetPairIndex = 0;
                }
                _packetPair[_packetPairIndex] = timeInterval;
            }
            // 5.记录包到达时间，存入ptk窗口
            if (_pktIndex < 0) {
                _pktIndex = 0;
            } else if (++_pktIndex >= HISTORY_WINDOW_SIZE) {
                _pktIndex = 0;
            }
            _pkt[_pktIndex] = arrivalTime;

            if (_receiveGlideWindow.tryReceivePacket(dataPacketPtr)) {
                int32_t estimatedSeqNumber = GlideWindow::increaseSeqNumber(_lrsn);  // 预测的序列号
                if (seqNumber != estimatedSeqNumber) {
                    int32_t left = _receiveGlideWindow.left();
                    int32_t right = _receiveGlideWindow.right();
                    bool normal = left < right;
                    bool greater = false;
                    if ((normal && (seqNumber > estimatedSeqNumber))) {
                        greater = true;
                    }

                    if (!normal) {
                        if (estimatedSeqNumber > left) {
                            estimatedSeqNumber = ~estimatedSeqNumber + 1;  // 负数
                        }
                        int32_t tmpSeqNumber = seqNumber;
                        if (tmpSeqNumber > left) {
                            tmpSeqNumber = ~tmpSeqNumber + 1;
                        }
                        if (tmpSeqNumber > estimatedSeqNumber) {
                            greater = true;
                        }
                    }

                    // 6.a 如果当前包序列号大于 lrsn+1，将两个序列号之间的所有序列号加入丢失列表，并且用一个nak包发送它们
                    if (greater) {
                        int32_t lossSeqNumber = estimatedSeqNumber;
                        uint64_t feebackTime = getCurrentTimeStampMicro();
                        std::vector<int32_t> nakList;
                        while (lossSeqNumber != seqNumber) {
                            nakList.push_back(lossSeqNumber);
                            _receiveLossList.addLossPacket(lossSeqNumber, feebackTime, left, right);
                            lossSeqNumber = GlideWindow::increaseSeqNumber(lossSeqNumber);
                        }
                        sendNakPacket(nakList);
                    } else if (seqNumber != _lrsn) {
                        // 6.a 如果序列号小于 lrsn，将它从接收端丢失列表中移除
                        _receiveLossList.removeLossPacket(seqNumber);
                    }
                }

                // 7. 更新lrsn
                _lrsn = seqNumber;
            } else {  // 序列号无法进入滑动窗口，就丢弃掉
                LOGW(TAG, "The seqNumber[%d] over receive glide window! drop it!\n", seqNumber);
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

    // 只负责数据包发送逻辑
    void UDTSocket::sendLoop() {
        LOGW(TAG, "Socket sendLoop running!");

        while (_running) {
            uint64_t beginTime = getCurrentTimeStampMicro();

            // 1.如果发送丢失列表不为空，重传丢失列表中的第一个包，并从列表中移除，转到5
            int32_t seqNumber = -1;
            bool sendAppData = true;
            if (_sendLossList.removeFirstLossPacket(seqNumber) && seqNumber >= 0) {  // 移除成功
                auto packetPtr = _sendGlideWindow.getPacket(seqNumber);
                if (packetPtr.get() != nullptr) {
                    sendDataPacket(packetPtr);
                }
                // 5.如果这个序列号是16n，转到2
                if (seqNumber % 16 != 0) {
                    sendAppData = false;
                }
            }

            while (_running && sendAppData) {
                // 2.todo:报文模式时，如果包存在丢失列表中时间超过应用指定的ttl生存时间，发送消息丢弃请求并且从丢失列表中移除所有相应的包，转到1
                if (_socketType == DGRAM) {

                }

                std::unique_lock<std::mutex> lock(_sndBufMutex);  // 发送缓冲锁
                // 3.等待直到有应用数据需要发送
                while (_running && _sndBuf.empty()) {
                    _sndCondition.wait(lock);
                }

                auto packetPtr = _sndBuf.front();

                std::unique_lock<std::mutex> lock_(_sendWindowMutex);  // 发送活动窗口锁
                // 4.b 能放入滑动窗口，打包成一个新的数据包并发送出去
                if (_sendGlideWindow.trySendPacket(packetPtr, _setupTime)) {
                    sendDataPacket(packetPtr);
                    _sndBuf.pop();
                } else { // 4.a.如果这个未被确认过的包无法放入滑动窗口，那么等待直到 ack包到来，转到1
                    lock.unlock();  // 先解锁前面的锁
                    waitAck = true;
                    while (_running && waitAck) {
                        _sndCondition.wait(lock_);
                    }
                    continue;
                }

                // 5.如果这个序列号是16n，转到2
                if (seqNumber % 16 != 0) {  // todo:这步是为了什么？
                    break;
                }
            }

            // 6.等待 snd - t 时间，snd 是发送间隔 t 是从第一步到第五步的时长，转到1
            std::unique_lock<std::mutex> lock(_sndMutex);  // 发送定时器锁
            if (_snd > 0) {
                while (_running && (getCurrentTimeStampMicro() < (beginTime + _snd))) {
                    _sndCondition.wait_for(lock, std::chrono::microseconds(beginTime + _snd));
                }
            }
        }

        LOGW(TAG, "Socket sendLoop stoped!");
    }

    void UDTSocket::receiveLoop() {
        LOGW(TAG, "Socket receiveLoop running!");

        while (_running) {
            // 1.查询系统时间检查ack、nak、exp定时器是否超时，如果任何一个超时，处理超时时间并重置相应的时间变量，对于ack，还要检查ack包间隔
            ackTask();
            nakTask();
            expTask();

            // 2.开始有限制的接收udp，如果没有包到达，转到1
            std::shared_ptr<UdpData> udpDataPtr = tryGetReceivedPacket();
            if (udpDataPtr.get() != nullptr) {
                // 2.1 重置连续超时次数为1，如果没有未响应的数据，或者这是一个ack或nak控制包，重置exp定时器
                _expCount = 1;
                UDTControlPacket::ControlPacketType controlPacketType = UDTControlPacket::ControlPacketType::UNKNOW;
                bool isControlPacket = UDTControlPacket::decodeControlTypeStatic(udpDataPtr->data(),
                                                                                 udpDataPtr->length(),
                                                                                 controlPacketType);
                if (_sendGlideWindow.empty() || (isControlPacket && (controlPacketType ==
                                                                     UDTControlPacket::ControlPacketType::ACK ||
                                                                     controlPacketType ==
                                                                     UDTControlPacket::ControlPacketType::NAK))) {
                    resetExpTimer();
                }
                // 3.检查是否是控制包，如果是，那么处理控制包，转到1
                if (isControlPacket) {  // 控制包
                    handleControlPacket(udpDataPtr, controlPacketType);
                } else {  // 数据包
                    handleDataPacket(udpDataPtr);
                }
            }
        }

        // 发送shutdown包
        if (_socketStatus != SHUTDOWN) {
            sendShutdownPacket();
        }

        LOGW(TAG, "Socket receiveLoop stoped!");
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

    void UDTSocket::sendDataPacket(std::shared_ptr<UDTDataPacket> dataPacketPtr) {
        if (_multiplexerPtr.get() != nullptr) {
            char tmpBuf[_maxPacketSize];
            int len = dataPacketPtr->encode(tmpBuf, _maxPacketSize);
            if (len > 0) {
                _multiplexerPtr->sendData(_remoteAddr, _remotePort, tmpBuf, (size_t) len);
            }
        }
    }

    void UDTSocket::resetExpTimer() {

    }
}