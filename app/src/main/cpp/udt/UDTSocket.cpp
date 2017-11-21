//
// Created by zhou rui on 2017/7/13.
//

#include "UDTSocket.h"
#include "UDTMultiplexer.h"
#include "packet/UDTKeepAlivePacket.h"
#include <math.h>//引入math头文件

namespace Dream {

    UDTSocket::UDTSocket(SocketType socketType, ConnectionMode connectionType,
                         std::shared_ptr<UDTMultiplexer> multiplexePtr, uint32_t socketId,
                         const std::string &remoteAddr, uint16_t remotePort,
                         fUDTDataCallBack dataCallBack, fUDTConnectStatusCallBack statusCallBack,
                         void *pUser, uint32_t dstSocketId, uint32_t maxPacketSize, uint32_t sendWindowSize,
                         uint32_t recvWindowSize,
                         uint32_t synPeriod, std::size_t sndBufSize, std::size_t recBufSize, std::size_t dgramPakcetMaxSize)
            : _socketStatus(UNCONNECTED), _socketType(socketType), _connectionType(connectionType),
              _seqNumber(-1), _maxPacketSize(maxPacketSize - 28),
              _sendWindowSize(sendWindowSize), _recvWindowSize(recvWindowSize), _synPeriod(synPeriod),
              _cookie(0), _multiplexerPtr(multiplexePtr), _socketId(socketId), _dstSocketId(dstSocketId),
              _remoteAddr(remoteAddr), _remotePort(remotePort), _dataCallBack(dataCallBack),
              _statusCallBack(statusCallBack), _user(pUser), _running(false),
              _largestACK2SeqNumber(-1), _ackHistoryWindow(16),
              _sndBufSize(sndBufSize), _sendGlideWindow(sendWindowSize),
              _recBufSize(recBufSize),
    _pktIndex(-1), _packetPairIndex(-1), _receiveGlideWindow(std::bind(&UDTSocket::reliablePacketCallback, this, std::placeholders::_1), recvWindowSize), _ackSeqNumber(-1), _rttUs(INIT_RTT_US), _rttVariance(INIT_RTT_US/2),
              _bufferSize(INIT_BUFFER_SIZE),
              _packetArrivalRate(0), _estimatedLinkCapacity(0), _snd(1), _waitAck(false), _lrsn(-1),
              _expCount(0), _expCountResetTime(0), _synTime(0), _ackTime(0), _nakTime(0),
              _expTime(0), _sndTime(0), _ackPeriod(0), _nakPeriod(0), _expPeriod(0),
              _handlePacket(false), _handling(false), _setupTime(0), _dgramBuf(nullptr), _dgramBufMaxLen(dgramPakcetMaxSize), _dgramBufLen(0), _slowStartPhase(true), _LastACK(-1), _LastDecPeriod(1), _AvgNAKNum(1), _NAKCount(1), _DecCount(1), _LastDecSeq(-1), _DecRandom(1) {

        updateAckAndNakPeriod();

        memset(_remoteAddrBytes, 0, 16);  // 初始化 这里不要写成长度4了，因为memset是按void*计算长度的
        memset(_localAddrBytes, 0, 16);

        // 远程地址
        if (!_remoteAddr.empty()) {
            uint32_t ip = 0;
            if (ipv4_2int(_remoteAddr, ip)) {
                _remoteAddrBytes[3] = ip;
            } else if (!ipv6_2int128(_remoteAddr, _remoteAddrBytes)) {
                LOGE(TAG, "Remote address is invaild! [%s]\n", _remoteAddr.c_str());
            }
        }

        // 本地地址
        const std::string localAddr = multiplexePtr->localAddress();
        
        if (!localAddr.empty()) {
            uint32_t ip = 0;
            if (ipv4_2int(localAddr, ip)) {
                _localAddrBytes[3] = ip;
            } else if (!ipv6_2int128(localAddr, _localAddrBytes)) {
                LOGE(TAG, "Local address is invaild! [%s]\n", localAddr.c_str());
            }
        }
        LOGW(TAG, "\n");
        
        memset(_pkt, 0, HISTORY_WINDOW_SIZE * sizeof(uint64_t));
        memset(_packetPair, 0, HISTORY_WINDOW_SIZE * sizeof(int64_t));
        
        if (_dgramBufMaxLen > 0) {
            _dgramBuf = new char[_dgramBufMaxLen];
        }
    }

    UDTSocket::~UDTSocket() {
//        shutdown();
        if (_dgramBuf) {
            delete [] _dgramBuf;
            _dgramBuf = nullptr;
        }
    }

    bool UDTSocket::isConnectionRequest(const char *pBuf, std::size_t len) {
        return (len >= 64) && ((uint8_t)pBuf[0] == 0x80) && ((uint8_t)pBuf[1] == 0x00);
    }
    
    uint32_t UDTSocket::socketId(const char* pBuf, std::size_t len) {
        if (len < 44) {
            return 0;
        }
        int pos = 40;
        uint32_t dstSocketId = 0;
        DECODE_INT32(pBuf + pos, dstSocketId, pos);
        return dstSocketId;
    }

    bool UDTSocket::postData(const char *pBuf, std::size_t len) {
        std::unique_lock<std::mutex> lock(_sndBufMutex);

        if (_sndBuf.size() >= _sndBufSize) {
            LOGE(TAG, "UDTSocket[%zd] id[%d]'s size over max size!\n", shared_from_this().get(), _socketId);
            return false;
        }

        std::size_t dataLen = _maxPacketSize - UDTDataPacket::headerLength();
        
        if (_socketType == STREAM) {  // 流 分割成定长的包
            int position = 0;
            while (position < len) {
                std::size_t copyLen = (position + dataLen < len) ? dataLen : (len - position);
//                _seqNumber = GlideWindow::increaseSeqNumber(_seqNumber);  没有链接时，这里会有问题
                auto packetPtr = std::make_shared<UDTDataPacket>(pBuf + position, copyLen, -1);
                _sndBuf.emplace(packetPtr);
                position += dataLen;
            }
        } else {  // 报文
            if (len <= dataLen) {  // 小于最大长度，用only
                auto packetPtr = std::make_shared<UDTDataPacket>(pBuf, len);
                _sndBuf.emplace(packetPtr);
            } else {  // 大于最大包长
                bool first = true;
                int position = 0;
                while (position < len) {
                    bool end = (position + dataLen >= len);
                    std::size_t copyLen = end ? (len - position) : dataLen;
                    UDTDataPacket::DataPacketType dataPacketType = UDTDataPacket::DataPacketType::MIDDLE;
                    if (first) {
                        dataPacketType = UDTDataPacket::DataPacketType::START;
                        first = false;
                    } else if (end) {
                        dataPacketType = UDTDataPacket::DataPacketType::STOP;
                    }
                    auto packetPtr = std::make_shared<UDTDataPacket>(pBuf + position, copyLen,
                                                                     -1, dataPacketType);
                    _sndBuf.emplace(packetPtr);
                    position += dataLen;
                }
            }
        }
        _sndCondition.notify_all();
        return true;
    }

    // 启动
    void UDTSocket::setup() {
        // 启动是由复用器调用，无需加锁
        _handlePacket = true;
        _running = true;
        // 开始发送线程和接收线程
        if (!_receiveThreadPtr.get()) {
            _receiveThreadPtr.reset(new std::thread(&UDTSocket::receiveLoop, this));
        }

        if (!_sendThreadPtr.get()) {
            _sendThreadPtr.reset(new std::thread(&UDTSocket::sendLoop, this));
        }
    }
    
    void UDTSocket::shutdownByReceiveLoop() {
        std::unique_lock<std::mutex> lock(_mutex);
        
        _handlePacket = false;
        
        _running = false;
        _sndCondition.notify_all();
        
        if (_sendThreadPtr.get()) {
            if (_sendThreadPtr->joinable()) {
                try {
                    _sendThreadPtr->join();
                } catch (...) {
                    
                }
            }
            _sendThreadPtr.reset();
        }
        
        if (_receiveThreadPtr.get()) {
            try {
                _receiveThreadPtr->detach();
            } catch (...) {
                
            }
//            _receiveThreadPtr.reset();
        }

        
        
        // 从复用器中删除
        auto sharedPtr = _multiplexerPtr.lock();
        if (sharedPtr.get()) {
            sharedPtr->removeSocketWithoutShutdown(_socketId);
        }
    }

    void UDTSocket::sendShutdownNotify() {
//        shutdownWithoutRemove();

        // 从复用器中删除
        auto sharedPtr = _multiplexerPtr.lock();
        if (sharedPtr.get()) {
            sharedPtr->removeSocket(_socketId);
        }
    }

    void UDTSocket::shutdownWithoutRemove() {
        _handlePacket = false;

        _running = false;
        _sndCondition.notify_all();
        
        if (_sendThreadPtr.get()) {
            if (_sendThreadPtr->joinable()) {
                try {
                    _sendThreadPtr->join();
                } catch (...) {
                    
                }
            }
            _sendThreadPtr.reset();
        }

        if (_receiveThreadPtr.get()) {
            if (std::this_thread::get_id() == _receiveThreadPtr->get_id()) {
                try {
                    _receiveThreadPtr->detach();
                } catch (...) {
                    
                }
            } else if (_receiveThreadPtr->joinable()) {
                try {
                    _receiveThreadPtr->join();
                } catch (...) {
                    
                }
                _receiveThreadPtr.reset();
            }
        }
    }

    void UDTSocket::shutdown() {
        std::unique_lock<std::mutex> lock(_mutex);

        shutdownWithoutRemove();

        // 从复用器中删除
        auto sharedPtr = _multiplexerPtr.lock();
        if (sharedPtr.get()) {
            sharedPtr->removeSocketWithoutShutdown(_socketId);
        }
    }

    std::string UDTSocket::getRemoteAddr() const {
        return _remoteAddr;
    }

    uint16_t UDTSocket::getRemotePort() const {
        return _remotePort;
    }
    
    uint32_t UDTSocket::getDstSocketId() const {
        return _dstSocketId;
    }

    bool UDTSocket::handleRevice() {
        return false;
    }

    void UDTSocket::synTask() {
        if (_running && synTimeout()) {
            tryConnectionSetup();
        }
    }

    // ack事件处理
    // ack序列号的发送原则，发送出的序列号是表示期待收到的序列号
    void UDTSocket::ackTask() {
        if (_running) {
            int32_t seqNumber = -1;
            // 1. seqNumber: 遵循以下 查找所有接收端收到的序列号，如果接收端丢失列表为空 lrsn+1, 否则使用丢失列表中最小的序列号
            if (_receiveLossList.empty()) {
                seqNumber = GlideWindow::increaseSeqNumber(_lrsn);  // _lrsn表示收到的最大序列号，如果接收丢失列表为空，那么就是期望接收_lrsn+1
            } else {
                seqNumber = _receiveLossList.getFirstSeqNumber();  // 丢失列表中的最小序列号就是期望接收的序列号
            }
            
//            LOGD(TAG, "ackTask seqnumber[%d]\n", seqNumber);
            // 2.a 如果这个ackNumber等于曾经通过ack2确认过的最大的ackNumber
            if (seqNumber == _largestACK2SeqNumber) {  // 已经被ack2确认过了，那么就不发送了，这里就不能使用3次ack快速重传的方案了
                return;
            }
            // 2.b 或者等于最后的ack并且两个ack包间隔小于2RTTs，不发送ack
            auto ackPtr = _ackHistoryWindow.getLastAckInfo();
            if (ackPtr.get() && seqNumber == ackPtr->seqNumber() &&
                ((getCurrentTimeStampMicro() - ackPtr->sendOutTime()) < (2 * _rttUs))) {
                return;
            }
            // 上面可以看出，2个rtts时间内只会发送一次，超过2个rtts后如果没收到ack2就不会再次发送了。
            
            // 3. ack序列号+1，将rtt，rtt variance，接收端流量窗口大小打包成ack包，如果这个确认不是通过ack定时器触发的，那么就发送并结束
            // 正常情况下，如果正常就收到包，那么每接收一个包都会响应一个ack包，除非是没有按顺序收到包，那样的话，前面应该已经发送过了一个nak包了，ack超时只是未了计算预测带宽用的？好像没有必要，不打算使用这个算法
            sendAckPacket(seqNumber, ackTimeout());  // 发送ack包中的数据包序列号的意义在于表示，直到该序列号前包括该序列号的数据包都已经收到
        }
    }

    void UDTSocket::nakTask() {
        if (_running && nakTimeout()) {
            // 搜索接收端丢失列表，找到上次反馈时间为k*RTT前的所有包，k初始值为2，包每次反馈k增长1，压缩并且发送这些序列号到一个nak包中
            std::vector<int32_t> lossList;
            if (_receiveLossList.serchNakTimeoutSeqNumber(lossList, getCurrentTimeStampMicro(), _rttUs)) {
                sendNakPacket(lossList);
            }
        }
    }

    bool UDTSocket::expTask() {
        if (_running && expTimeout()) {
            // 1. 将所有未响应的包放入发送端丢失列表中
            bool lossListIsEmpty = _sendGlideWindow.checkExpAndLossNothing(_sendLossList);
            
            // 这里改变了发送丢失列表，需要唤醒发送线程去重传数据
            {
                std::unique_lock<std::mutex> lock(_sendWindowMutex);
                if (_waitAck) {
                    _waitAck = false;
                    _sndCondition.notify_all();
                }
            }

            // 2.如果(exp-count>16)并且自上次从对方接收到一个包以来的总时间超过3秒，或者这个时间已经超过3分钟了，这被认为是连接已经断开，关闭UDT连接。
            uint64_t currentTime = getCurrentTimeStampMicro();
            if ((_expCount > 16 && (_expCountResetTime + 3000000 <= currentTime)) ||
                (_expCountResetTime + 60 * 3000000 <= currentTime)) {
                // todo:断开udt连接
//                std::unique_ptr<std::thread> threadPtr;
//                threadPtr.reset(new std::thread(&UDTSocket::shutdown, this));
                sendShutdownNotify();
                return true;
            }

            if (lossListIsEmpty) {
                sendKeepAlivePacket();
            } else {
//                LOGD(TAG, "UDTSocket[%zd] id[%d] sender's loss list not empty!\n", shared_from_this().get(), _socketId);

            }

            ++_expCount;
        }

        return false;
    }

    void UDTSocket::sndTask() {

    }

    inline void UDTSocket::updateRtt(int32_t rttUs) {
        _rttUs = (_rttUs * 7 + rttUs) / 8;
    }

    inline void UDTSocket::updateRttVariance(int32_t rttUs) {  // 获取rtt样本方差
        _rttVariance = (_rttVariance * 3 + abs(_rttUs - rttUs)) / 4;
    }

    inline void UDTSocket::updateAckAndNakPeriod() {
        uint32_t period = _rttUs + 4 * _rttVariance + SYN_TIME_US;  // 文档上的 4*RTT+RTTVAR是写错了，应该是RTT+4*RTTVAR
        _ackPeriod = period > SYN_TIME_US ? SYN_TIME_US : period;
        _nakPeriod = period;
    }


    inline void UDTSocket::updatePacketArrivalRate(int32_t rate) {
        if (rate <= 0) {
            return;
        }
        if (_packetArrivalRate == 0) {
            _packetArrivalRate = rate;
        } else {
            _packetArrivalRate = (_packetArrivalRate * 7 + rate) / 8;
        }
    }

    inline void UDTSocket::updateEstimatedLinkCapacity(int32_t capacity) {
        if (capacity <= 0) {
            return;
        }
        if (_estimatedLinkCapacity == 0) {
            _estimatedLinkCapacity = capacity;
        } else {
            _estimatedLinkCapacity = (_estimatedLinkCapacity * 7 + capacity) / 8;
        }
    }

    inline uint32_t UDTSocket::getExpPeriod() {
        unsigned period = _expCount * _nakPeriod;
        return period < EXP_PERIOD_MIN ? EXP_PERIOD_MIN : period;
    }

    inline bool UDTSocket::synTimeout() {
        if (_socketStatus == CONNECTED) {
            return false;
        }

        uint64_t currentTime = getCurrentTimeStampMicro();
        if (_synTime > 0 && (_synTime + _synPeriod > currentTime)) {
            return false;
        } else {
            _synTime = currentTime;
            return true;
        }
    }

    inline bool UDTSocket::ackTimeout() {
        uint64_t currentTime = getCurrentTimeStampMicro();
        if (_ackTime > 0 && (_ackTime + _ackPeriod > currentTime)) {  // 没超时
            return false;
        } else {
            _ackTime = currentTime;
            return true;
        }
    }

    inline bool UDTSocket::nakTimeout() {
        uint64_t currentTime = getCurrentTimeStampMicro();
        if (_nakTime > 0 && (_nakTime + _nakPeriod > currentTime)) {  // 没超时
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

    inline bool UDTSocket::addressIsEquals(const uint32_t *a, const uint32_t *b) {
        bool isEquals = true;
        for (std::size_t i = 0; i < 4; ++i) {
            if (a[i] != b[i]) {
                isEquals = false;
                break;
            }
        }
        return isEquals;
    }

    inline void UDTSocket::generateCookie() {
        _cookie = (uint32_t) ((_remoteAddrBytes[0] - _remoteAddrBytes[1] + _remoteAddrBytes[2] -
                               _remoteAddrBytes[3]) * COOKIE_SECRET_KEY + COOKIE_SECRET_KEY +
                              getCurrentTimeStampMicro());
    }
    
    void UDTSocket::reliablePacketCallback(std::shared_ptr<UDTDataPacket> udtDataPacketPtr) {
        if (_dataCallBack) {
            if (_socketType == STREAM || udtDataPacketPtr->dataPacketType() == UDTDataPacket::DataPacketType::ONLY) {  // 如果是码流类型或者是only报文的话，就直接回调给上层
                _dgramBufLen = 0;  // 清空原来的缓存
               _dataCallBack(_socketId, udtDataPacketPtr->data(), udtDataPacketPtr->len(), _user);
            } else {  // 如果是除only以外的报文类型的话，还需要拼包判断
                std::size_t packetBufLen = udtDataPacketPtr->len();
                if (packetBufLen + _dgramBufLen > _dgramBufMaxLen) {
                    LOGE(TAG, "The dgram pakcet size over max size[%zd], missing data!\n", _dgramBufMaxLen);
                    _dgramBufLen = 0;
                }
                const char* packetBuf = udtDataPacketPtr->data();
                if (!packetBuf) {
                    LOGE(TAG, "The dgram packet buf is NULL!\n");
                }
                UDTDataPacket::DataPacketType dataPacketType = udtDataPacketPtr->dataPacketType();
                if (dataPacketType == UDTDataPacket::DataPacketType::START) {  // 开始
                    _dgramBufLen = 0;
                }
                memcpy(_dgramBuf + _dgramBufLen, packetBuf, packetBufLen);  // 拷贝数据
                _dgramBufLen += packetBufLen;  // 增加长度
                
                if (dataPacketType == UDTDataPacket::DataPacketType::STOP) {  // 如果结束了
                    // 回调给上层
                    _dataCallBack(_socketId, _dgramBuf, _dgramBufLen, _user);
                    // 重置长度
                    _dgramBufLen = 0;
                }
            }
        }
    }

    inline void UDTSocket::initSocket(const UDTSynPacket &packet) {
        // 比较自己的maxwindowsSize和maxpacketsize
        uint32_t maxPacketSize = packet.maxPacketSize();
        uint32_t maxWindowSize = packet.maxWindowSize();
        if (maxPacketSize < _maxPacketSize) {
            _maxPacketSize = maxPacketSize;
        }
        if (_sendWindowSize > maxWindowSize) {
            _sendWindowSize = maxWindowSize;
        }
        _lrsn = packet.initSeqNumber();
        _largestACK2SeqNumber = _lrsn;
        _sendGlideWindow.setWindowSize(_sendWindowSize);
        _receiveGlideWindow.setInitSeqNumber(_lrsn);

        changeSocketStatus(CONNECTED);
    }

    // 处理握手包
    void UDTSocket::handleSynPacket(const UDTSynPacket &packet) {
        if (_socketStatus == CONNECTED || _socketStatus == SHUTDOWN) {  // 已连接或已关闭状态，就不再接收连接请求
            LOGE(TAG, "UDTSocket[%zd] id[%d] is connected or closed already!\n", shared_from_this().get(), _socketId);
            return;
        }

        // 验证
        if (!addressIsEquals(packet.addressBytes(), _localAddrBytes)) {  // 地址不同
#ifdef DEBUG
            auto sharedPtr = _multiplexerPtr.lock();
            if (sharedPtr.get()) {
                LOGE(TAG, "UDTSocket[%zd] id[%d] packet address bytes is't equals local address! localAddr[%s] localPort[%d] localSocket[%d], packet's address bytes is [0x%02x 0x%02x 0x%02x 0x%02x], local's address bytes is [0x%02x 0x%02x 0x%02x 0x%02x]\n", shared_from_this().get(), _socketId, sharedPtr->localAddress().c_str(), sharedPtr->localPort(), _socketId, packet.addressBytes()[0], packet.addressBytes()[1], packet.addressBytes()[2], packet.addressBytes()[3], _localAddrBytes[0], _localAddrBytes[1], _localAddrBytes[2], _localAddrBytes[3]);
            }
#endif
            return;
        }
        
        if (_dstSocketId == 0) {
            _dstSocketId = packet.socketId();
        }

        int32_t connectionType = packet.connectionType();
        switch (connectionType) {
            case RENDEZVOUS_REQUEST:  // p2p方式接收
                if (_connectionType == RENDEZVOUS) {
                    sendSynPacket(FIRST_RESPONSE);  // 发送响应包
                }
                break;
            case CLIENT_REQUEST:
                if (_connectionType == SERVER) {  // 服务端接收客户端请求
                    // 第一次接收到客户端的连接请求，生成cookie，返回给客户端，客户端然后必须发送回相同的cookie给服务器
                    if (_cookie == 0) {
                        generateCookie();
                    } else if (_cookie == packet.synCookie()) {  // 验证cookie相同
                        // 比较自己的maxwindowsSize和maxpacketsize
                        uint32_t maxPacketSize = packet.maxPacketSize();
                        uint32_t maxWindowSize = packet.maxWindowSize();
                        if (maxPacketSize < _maxPacketSize) {
                            _maxPacketSize = maxPacketSize;
                        }
                        if (_sendWindowSize > maxWindowSize) {
                            _sendWindowSize = maxWindowSize;
                        }
                        _lrsn = packet.initSeqNumber();
                        _largestACK2SeqNumber = _lrsn;
                        _sendGlideWindow.setWindowSize(_sendWindowSize);
                        _receiveGlideWindow.setInitSeqNumber(_lrsn);

                        changeSocketStatus(CONNECTED);
                    }
                    sendSynPacket(FIRST_RESPONSE);
                }
                break;
            case FIRST_RESPONSE:
                if (_connectionType == CLIENT) {
                    // 获得cookie
                    uint32_t cookie = packet.synCookie();
                    if (cookie != 0) {  // 第一次收到cookie
                        if (_cookie == 0) {
                            _cookie = cookie;
                            sendSynPacket(CLIENT_REQUEST);  // 重发请求
                        } else {  // 第二次收到，连接建立成功，可以发送数据
                            initSocket(packet);
                        }
                    }
                } else if (_connectionType != RENDEZVOUS) {
                    // 初始化数据
                    initSocket(packet);
                    sendSynPacket(RENDEZVOUS);
                }
                break;
            case SECOND_RESPONSE:
                // nothing
            default:
                // nothing
                break;
        }
    }

    void UDTSocket::handleShutdownPacket(const UDTShutdownPacket &packet) {

    }

    void UDTSocket::handleAckPacket(const UDTAckPacket &packet) {
        int32_t seqNumber = packet.seqNumber();
        LOGD(TAG, "UDTSocket[%zd] id[%d] handle ACK packet, seq number is [%d], ack number is [%d]\n", shared_from_this().get(), _socketId, seqNumber, packet.getAdditionalInfo());
    
        seqNumber = GlideWindow::decreaseSeqNumber(seqNumber);
        
        // 1. 更新最大响应序列号
        _sendGlideWindow.ackPacket(seqNumber);
        
        // 唤醒发送线程
        {
            std::unique_lock<std::mutex> lock(_sendWindowMutex);
            _waitAck = false;
            _sndCondition.notify_all();
        }

        // 2. 发送回一个包含相同ack序列号的ack2包
        sendAck2Packet(packet.getAdditionalInfo());

        // 3. 更新rtt和rttvar
        _rttUs = packet.rttUs();
        _rttVariance = packet.rttVariance();

        // 4. 更新所有ack和nak的周期为 4*RTT + RTTVar + SYN.
        updateAckAndNakPeriod();

        // 5. todo:更新流量窗口大小
//        _sendGlideWindow.setWindowSize(packet.availableBufSize());

        // 6. 如果这是一个轻的ack，没有包到达速度和链路容量的话，就停止
        int32_t receiveRate = packet.receiveRate();
        int32_t linkCapacity = packet.linkCapacity();
        if (receiveRate >= 0 || linkCapacity >= 0) {
            // 7. 更新包到达速度
            updatePacketArrivalRate(receiveRate);
            
            // 8. 更新估算链路容量
            updateEstimatedLinkCapacity(linkCapacity);
            
            LOGD(TAG, "UDTSocket[%zd] id[%d] update packetArrivalRate[%d], estimatedLinkCapacity[%d]\n", shared_from_this().get(), _socketId, _packetArrivalRate, _estimatedLinkCapacity);
            
            // 9. 释放发送端缓存中已被确认的包
            // todo:合到第一步中
        }
        
        // 原生流量控制
        // 1) 如果当前状态是慢启动状态，设置拥塞窗口大小为 包到达速率 * （RTT + SYN)，慢启动结束
        const double minInc = 0.01;  // 最小的inc值
        
        if (_slowStartPhase) {
            _sendWindowSize += seqlen(_LastACK, seqNumber);
            _sendGlideWindow.setWindowSize(_sendWindowSize);
            _LastACK = seqNumber;
            _slowStartPhase = false;
            
            if (_packetArrivalRate > 0) {
                _snd = 1000000.0 / _packetArrivalRate;
            } else {
                _snd = (_rttUs + SYN_TIME_US) / _sendWindowSize;
            }
        } else {
            // 2) 设置拥塞窗口大小(CWND), CWND = A * (RTT + SYN) + 16
            _sendWindowSize = _packetArrivalRate / 1000000.0 * (_rttUs + SYN_TIME_US) + 16;
            _sendGlideWindow.setWindowSize(_sendWindowSize);
            
            // 3)
            int64_t B = _estimatedLinkCapacity - 1000000.0 / _snd;
            double inc = 0;
            if ((_snd > _LastDecPeriod) && ((_estimatedLinkCapacity / 9) < B)) {
                B = _estimatedLinkCapacity / 9;
            }
            if (B <= 0) {
                inc = minInc;
            } else {
                inc = pow(10.0, ceil(log10(B * _maxPacketSize * 8.0))) * 0.0000015 / _maxPacketSize;
                
                if (inc < minInc) {
                    inc = minInc;
                }
            }
            
            // 4)
            _snd = (_snd * SYN_TIME_US) / (_snd * inc + SYN_TIME_US);
            LOGD(TAG, "UDTSocket[%zd] id[%d] update SND to [%d], current window size [%d]\n", shared_from_this().get(), _socketId, _snd, _sendWindowSize);
        }

        
        // 10. 更新发送端丢失列表
        _sendGlideWindow.removeLossPacket(_sendLossList);
    }

    void UDTSocket::handleNakPacket(const UDTNakPacket &packet) {
        // 1. 添加所有nak负载的序列号到发送者的丢失列表
        const int32_t *lossSeqNumber = packet.lossSeqNumber();
        std::size_t lossSize = packet.lossSize();
        LOGD(TAG, "UDTSocket[%zd] id[%d] handle NAK packet\n", shared_from_this().get(), _socketId);

        int32_t beginSeqNumber = -1;
        bool haveBegin = false;
        for (size_t i = 0; i < lossSize; ++i) {
            int32_t tmp = lossSeqNumber[i];
            int32_t seqNumber = tmp & 0x7fffffff;
            if (haveBegin && beginSeqNumber >= 0) {  // 前面有的话就把中间的全部加进去
                int32_t middleSeqNumber = GlideWindow::increaseSeqNumber(beginSeqNumber);
                while (middleSeqNumber != seqNumber) {
                    _sendGlideWindow.nakPacket(_sendLossList, middleSeqNumber);
                    middleSeqNumber = GlideWindow::increaseSeqNumber(middleSeqNumber);
                }
                haveBegin = false;
                beginSeqNumber = -1;
            } else {
                haveBegin = ((tmp & 0x80000000) >> 31) == 1;
                if (haveBegin) {
                    beginSeqNumber = seqNumber;
                }
            }
            _sendGlideWindow.nakPacket(_sendLossList, seqNumber);
        }
        
        // 唤醒发送线程
        {
            std::unique_lock<std::mutex> lock(_sendWindowMutex);
            _waitAck = false;
            _sndCondition.notify_all();
        }

        // 2. todo:通过速率控制更新snd周期
        // 我们定义一个阻塞时期为：
        // 第一个丢失包序列号大于LastDecSeq(上次发送速度降低时的最大序列号)的两个NAKs间的时期。
        // AvgNAKNum是在阻塞时期NAKs的平均数量
        // NAKCount是在当前时期NAKs的当前数量
        // 1) 如果是慢启动阶段，设置包间隔为1/接收速度，慢启动结束，停止
        if (_slowStartPhase) {
            _slowStartPhase = false;
            if (_packetArrivalRate > 0) {
                _snd = 1000000.0 / _packetArrivalRate;
            } else {
                _snd = _sendWindowSize / (_rttUs + SYN_TIME_US);
            }
        } else {
            // 2) 如果这个NAKs开始了一个新的阻塞时期，增加包间隔(snd)为snd = snd * 1.125；更新AvgNAKNum，重置NAKCount为1，并且计算DecRandom为一个随机值（概率平均分布）从1到AvgNAKNum之间。更新LastDecSeq，停止
            if (seqcmp(lossSeqNumber[0] & 0x7fffffff, _LastDecSeq) > 0) {
                _snd = ceil(_snd * 1.125);
                _AvgNAKNum = ceil(_AvgNAKNum * 0.875 + _NAKCount * 0.125);
                _NAKCount = 1;
                _DecCount = 1;
                
                _LastDecSeq = _seqNumber;
                
                srand(_LastDecSeq);
                _DecRandom = ceil(_AvgNAKNum * (double(rand() / RAND_MAX)));
                if (_DecRandom < 1) {
                    _DecRandom = 1;
                }
            } else if ((_DecCount++ < 5) && (0 == (++_NAKCount % _DecRandom))) {
                // 3) 如果DecCount <= 5，并且NAKCount == DecCount * DecRandom:
                // a. 更新SND周期：SND = SND * 1.125；
                // b. DecCount增加1；
                // c. 记录这个当前最大发送序列号（LastDecSeq）。
                _snd = ceil(_snd * 1.125);
                _LastDecSeq = _seqNumber;
            }
        }
        
        // 3. 重置exp时间变量
        _expTime = getCurrentTimeStampMicro();
    }

    void UDTSocket::handleAck2Packet(const UDTAck2Packet &packet) {
        // 1. 按照ack2包中的ack序列号找出在ack历史窗口中对应的ack
        int32_t ackSeqNumber = packet.getAdditionalInfo();
//        LOGD(TAG, "UDTSocket[%zd] id[%d] handle ACK2 packet, ack number is [%d]\n", shared_from_this().get(), _socketId, ackSeqNumber);
        
        std::shared_ptr<AckInfo> ackInfoPtr = _ackHistoryWindow.getAckInfoBySeqNumber(ackSeqNumber);
        if (ackInfoPtr.get()) {
            int32_t seqNumber = ackInfoPtr->seqNumber();
            // 2. 更新曾经被确认的最大确认序号，这里应该是数据包的序列号
            if (_largestACK2SeqNumber < 0) {
                _largestACK2SeqNumber = seqNumber;
            } else {
                int32_t offset = seqNumber - _largestACK2SeqNumber;
                if ((offset > 0 && offset < INT32_MAX / 2) || (offset < 0 && offset < INT32_MIN / 2)) {
                    _largestACK2SeqNumber = seqNumber;
                }
            }

            // 3. 通过ack2到达时间和ack离去时间计算新的rtt，并且更新rtt值为：RTT = (RTT * 7 + rtt) / 8
            int32_t rtt = (int32_t) (getCurrentTimeStampMicro() - ackInfoPtr->sendOutTime());
            if (rtt < 0) {
                LOGE(TAG, "rtt < 0, must is failed!\n");
                return;
            }
            
            // 4. 计算RTTVar通过：RTTVar = (RTTVar * 3 + abs(RTT - rtt)) / 4
            updateRttVariance(rtt);
            
            updateRtt(rtt);  // 这里和更新RTTVar调换一下顺序，因为先更新掉rtt的话会影响到RTTVar的计算公式

            // 5. 更新所有ack和nak周期为 4 * RTT + RTTVar + SYN
            updateAckAndNakPeriod();
        }
    }

    void UDTSocket::handleMDRPacket(const UDTMDRPacket &packet) {
        // 1. todo: 标记在接收缓冲中属于这个消息的所有包，让它们不会被读

        // 2. todo: 移除在接收丢失队列中所有相应的包
    }

    void UDTSocket::addTimestampAndDstSocketId(UDTBasePacket &basePacket) {
        basePacket.setTimestamp(
                (uint32_t) (_setupTime == 0 ? 0 : getCurrentTimeStampMicro() - _setupTime));
        basePacket.setDstSocketId(_dstSocketId);
    }

    void UDTSocket::sendSynPacket(int32_t connectionType) {
        auto sharedPtr = _multiplexerPtr.lock();
        if (sharedPtr.get()) {
            if (_seqNumber < 0) {
                _seqNumber = rangeRand(0, UDTBasePacket::SEQUENCE_NUMBER_MAX);
                _LastACK = GlideWindow::decreaseSeqNumber(_seqNumber);
                LOGD(TAG, "UDTSocket[%zd] id[%d] generate seq number [%d]\n", shared_from_this().get(), _socketId, _seqNumber);
            }
            
            UDTSynPacket packet(UDT_VERSION, _socketType, _seqNumber, _maxPacketSize,
                                _recvWindowSize, connectionType, _socketId, _cookie,
                                _remoteAddrBytes);
            addTimestampAndDstSocketId(packet);

            char tmpBuf[_maxPacketSize];
            int len = packet.encode(tmpBuf, _maxPacketSize);
            if (len > 0) {
                sharedPtr->sendData(_remoteAddr, _remotePort, tmpBuf, (size_t) len);
            }
            // 更新时间
            _synTime = getCurrentTimeStampMicro();
        }
    }

    void UDTSocket::sendKeepAlivePacket() {
//        LOGD(TAG, "UDTSocket[%zd] id[%d] send KEEPALIVE packet\n", shared_from_this().get(), _socketId);
        auto sharedPtr = _multiplexerPtr.lock();
        if (sharedPtr.get()) {
            UDTKeepAlivePacket packet;
            addTimestampAndDstSocketId(packet);

            char tmpBuf[_maxPacketSize];
            int len = packet.encode(tmpBuf, _maxPacketSize);
            if (len > 0) {
                sharedPtr->sendData(_remoteAddr, _remotePort, tmpBuf, (size_t) len);
            }
        }
    }

    void UDTSocket::sendAckPacket(int32_t seqNumber, bool byAckTimer) {
        auto sharedPtr = _multiplexerPtr.lock();
        if (sharedPtr.get()) {
            if (_ackSeqNumber < 0) {
                _ackSeqNumber = rangeRand(0, UDTControlPacket::ADDITIONAL_INFO_MAX);
            } else {
                if (_ackSeqNumber == UDTControlPacket::ADDITIONAL_INFO_MAX) {
                    _ackSeqNumber = 0;
                } else {
                    ++_ackSeqNumber;
                }
            }

            int32_t receiveRate = -1;
            int32_t linkCapacity = -1;
            if (byAckTimer) {
                // 4. 计算包到达速度
                if (_pktIndex >= 0) {
                    _medianUtil.reset();

                    int beginIndex = _pktIndex + 1;
                    if (beginIndex == HISTORY_WINDOW_SIZE) {
                        beginIndex = 0;
                    }
                    int i = 0;
                    uint64_t preArrivalTime = 0;
                    for (; i < HISTORY_WINDOW_SIZE; ++i) {
                        uint64_t arrivalTime = _pkt[beginIndex];
                        if (arrivalTime == 0) {  // 如果有包到达速度等于0的，那么达不到16个变量的条件
                            break;
                        }
                        if (preArrivalTime > 0) {
                            _medianUtil.add(arrivalTime - preArrivalTime);
                        }
                        preArrivalTime = arrivalTime;
                        if (++beginIndex == HISTORY_WINDOW_SIZE) {
                            beginIndex = 0;
                        }
                    }
                    if (i == HISTORY_WINDOW_SIZE) {
                        int64_t ai = _medianUtil.getMedianValue();
                        receiveRate = _medianUtil.getPacketArrivalSpeed(ai);
                    }
                }

                // 5. 计算预测链路容量
                if (_packetPairIndex >= 0) {
                    _medianUtil.reset();

                    int beginIndex = _packetPairIndex + 2;
                    if (beginIndex == HISTORY_WINDOW_SIZE) {
                        beginIndex = 0;
                    } else if (beginIndex == HISTORY_WINDOW_SIZE + 1) {
                        beginIndex = 1;
                    }
                    int i = 0;
                    for (; i < 16; ++i) {
                        int64_t packetPairTime = _packetPair[beginIndex];
                        if (packetPairTime == 0) {  // 如果有包对间隔等于0的，那么达不到16个变量的条件
                            break;
                        }
                        _medianUtil.add(packetPairTime);
                        if (++beginIndex == HISTORY_WINDOW_SIZE) {
                            beginIndex = 0;
                        }
                    }
                    if (i == 16) {
                        int64_t pi = _medianUtil.getMedianValue();
                        linkCapacity = _medianUtil.getEstimatedLinkCapacity(pi);
                    }
                }

            }
            
            LOGD(TAG, "UDTSocket[%zd] id[%d] receiveRate[%d], linkCapacity[%d], send ACK packet, ack seq number is [%d], ack number is [%d]\n", shared_from_this().get(), _socketId, receiveRate, linkCapacity, _ackSeqNumber, seqNumber);

            // 6. 将包达到速度和预测链路容量打包到ack包中
//            LOGD(TAG, "UDTSocket[%zd] id[%d] send ACK packet, ack seq number is [%d], ack number is [%d]\n", shared_from_this().get(), _socketId, _ackSeqNumber, seqNumber);
            UDTAckPacket packet(_ackSeqNumber, seqNumber, _rttUs, _rttVariance,
                                _receiveGlideWindow.getWindowSize(), receiveRate, linkCapacity);
            addTimestampAndDstSocketId(packet);

            char tmpBuf[_maxPacketSize];
            int len = packet.encode(tmpBuf, _maxPacketSize);
            if (len > 0) {
                sharedPtr->sendData(_remoteAddr, _remotePort, tmpBuf, (size_t) len);
            }

            // 7. 记录这个ack序列号，确认号，离开时间到ack历史窗口中
            _ackHistoryWindow.addAck(_ackSeqNumber, seqNumber, getCurrentTimeStampMicro());
        }
    }

    void UDTSocket::sendNakPacket(const std::vector<int32_t> &seqNumberList) {
        auto sharedPtr = _multiplexerPtr.lock();
        if (sharedPtr.get()) {

            std::size_t size = seqNumberList.size();
            int32_t tmpArray[size];
            std::size_t arrayPos = 0;
            int32_t preSeqNmber = -1;
            bool isContinuous = false;
            for (std::size_t i = 0; i < size; ++i) {
                int32_t eleSeqNumber = seqNumberList[i];
                if (preSeqNmber >= 0 && arrayPos > 0 &&
                    GlideWindow::continuousSeqNumber(preSeqNmber, eleSeqNumber)) {  // 是连续的序列号
                    // 将前面的最高位置为1
                    if (!isContinuous) {
                        tmpArray[arrayPos - 1] |= (0x80 << 24);
                        isContinuous = true;  // 已经标志为连续的
                    } else {
                        // 这种情况就压缩了
                    }
                } else {  // 不连续或者是第一个
                    if (preSeqNmber >= 0) {  // 如果不是第一个的话，就写上结尾
                        tmpArray[arrayPos++] = preSeqNmber;
                    }
                    tmpArray[arrayPos++] = eleSeqNumber;  // 写上当前序列号
                    isContinuous = false;
                }
                preSeqNmber = eleSeqNumber;
            }
            
            if (isContinuous && preSeqNmber >= 0) { // 一直连续的话，没有闭合
                tmpArray[arrayPos++] = preSeqNmber;
            }
            
            LOGD(TAG, "------------UDTSocket[%zd] id[%d] send NAK packet, seq number list size[%lu] arrayPos[%zd]------------------------------------------\n", shared_from_this().get(), _socketId, size, arrayPos);

            UDTNakPacket packet(tmpArray, arrayPos);
            addTimestampAndDstSocketId(packet);
            char tmpBuf[_maxPacketSize];
            int len = packet.encode(tmpBuf, _maxPacketSize);
            if (len > 0) {
                sharedPtr->sendData(_remoteAddr, _remotePort, tmpBuf, (size_t) len);
            }
        }
    }

    void UDTSocket::sendAck2Packet(int32_t ackSeqNumber) {
        auto sharedPtr = _multiplexerPtr.lock();
        if (sharedPtr.get()) {

            UDTAck2Packet packet(ackSeqNumber);
            addTimestampAndDstSocketId(packet);

            char tmpBuf[_maxPacketSize];
            int len = packet.encode(tmpBuf, _maxPacketSize);
            if (len > 0) {
                sharedPtr->sendData(_remoteAddr, _remotePort, tmpBuf, (size_t) len);
            }
        }
    }

    // 发送停止包
    void UDTSocket::sendShutdownPacket() {
        auto sharedPtr = _multiplexerPtr.lock();
        if (sharedPtr.get()) {
            UDTShutdownPacket packet;
            addTimestampAndDstSocketId(packet);
            char tmpBuf[_maxPacketSize];
            int len = packet.encode(tmpBuf, _maxPacketSize);
            if (len > 0) {
                sharedPtr->sendData(_remoteAddr, _remotePort, tmpBuf, (size_t) len);
            }
        }
    }

    void UDTSocket::sendMDRPacket() {
        // todo:暂时没想好怎么写
    }

    void UDTSocket::receiveData(const std::string &remote_address, uint16_t remote_port,
                                const char *pBuf, std::size_t len) {
        // 地址变了是有可能的，如果地址变了，那么就要求重新连接
        if (remote_address != _remoteAddr) {
            LOGW(TAG, "UDTSocket[%zd] id[%d] Receive data from [%s], is not equals [%s]\n", shared_from_this().get(), _socketId, remote_address.c_str(), _remoteAddr.c_str());
            return;
        }

        if (_handlePacket) {
            std::unique_lock<std::mutex> lock(_recBufMutex);
            if (_recBuf.size() < _recBufSize) {
                auto udpPtr = std::make_shared<UdpData>(remote_address, remote_port, len, pBuf);
                _recBuf.emplace(udpPtr);
//                LOGD(TAG, "UDTSocket[%zd] id[%d] add recBuf is success! current size[%lu]\n", shared_from_this().get(), _socketId, _recBuf.size());
            } else {
                LOGE(TAG, "UDTSocket[%zd] id[%d] recbuf is full!\n", shared_from_this().get(), _socketId);
            }
        }
    }

    std::shared_ptr<UdpData> UDTSocket::tryGetReceivedPacket() {
        std::unique_lock<std::mutex> lock(_recBufMutex);

        std::shared_ptr<UdpData> udpDataPtr;
        if (!_recBuf.empty()) {
            udpDataPtr = _recBuf.front();
            _recBuf.pop();
//            LOGD(TAG, "tryGetReceivedPacket size[%lu]\n", _recBuf.size());
        }
        return udpDataPtr;
    }

    void UDTSocket::handleControlPacket(std::shared_ptr<UdpData> udpDataPtr,
                                        const UDTControlPacket::ControlPacketType &controlPacketType) {
//        LOGD(TAG, "UDTSocket[%zd] id[%d] handle control packet!\n", shared_from_this().get(), _socketId);
//
        const char *pBuf = udpDataPtr->data();
        std::size_t len = udpDataPtr->length();

        if (_socketStatus == CONNECTED &&
            controlPacketType == UDTControlPacket::ControlPacketType::ACK) {
            UDTAckPacket ackPacket;
            ackPacket.decode(pBuf, len);
            handleAckPacket(ackPacket);
        } else if (_socketStatus == CONNECTED &&
                   controlPacketType == UDTControlPacket::ControlPacketType::NAK) {
            UDTNakPacket nakPacket;
            nakPacket.decode(pBuf, len);
            handleNakPacket(nakPacket);
        } else if (_socketStatus == CONNECTED &&
                   controlPacketType == UDTControlPacket::ControlPacketType::ACK2) {
            UDTAck2Packet ack2Packet;
            ack2Packet.decode(pBuf, len);
            handleAck2Packet(ack2Packet);
        } else if (controlPacketType == UDTControlPacket::ControlPacketType::KEEP_ALIVE) {
            // Do nothing.
//            LOGD(TAG, "UDTSocket[%zd] id[%d] Receive KEEPALIVE\n", shared_from_this().get(), _socketId);
        } else if (_socketStatus != CONNECTED && _socketStatus != SHUTDOWN &&
                   controlPacketType == UDTControlPacket::ControlPacketType::SYN) {
            UDTSynPacket synPacket;
            synPacket.decode(pBuf, len);
            handleSynPacket(synPacket);
        } else if (controlPacketType == UDTControlPacket::ControlPacketType::SHUTDOWN) {
            UDTShutdownPacket shutdownPacket;
            shutdownPacket.decode(pBuf, len);
            handleShutdownPacket(shutdownPacket);
        } else {
            LOGE(TAG, "UDTSocket[%zd] id[%d], The control type [%d] is unsupported, current socket status[%d]!\n",
                 shared_from_this().get(), _socketId, controlPacketType, _socketStatus);
        }
    }

    void UDTSocket::handleDataPacket(std::shared_ptr<UdpData> udpDataPtr) {
        if (_socketStatus != CONNECTED) {
            LOGE(TAG, "UDTSocket[%zd] id[%d] handle data packet failed! seq socket status is[%d]\n", shared_from_this().get(), _socketId, _socketStatus);
            return;
        }

        auto dataPacketPtr = std::make_shared<UDTDataPacket>();
        if (dataPacketPtr->decode(udpDataPtr->data(), udpDataPtr->length()) > 0) {
            int32_t seqNumber = dataPacketPtr->seqNumber();
            if (_receiveGlideWindow.tryReceivePacket(dataPacketPtr)) {
                // 4.如果当前包的序列号是 16n+1，记录这个包跟上次最后一个数据包的时间间隔，存入包对窗口
                LOGW(TAG, "UDTSocket[%zd] id[%d] handle data packet! seq number[%d] lrsn[%d]\n", shared_from_this().get(), _socketId, seqNumber, _lrsn);
                
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

                int32_t estimatedSeqNumber = GlideWindow::increaseSeqNumber(_lrsn);  // 预测的序列号
                bool greaterLrsn = true;
                if (seqNumber != estimatedSeqNumber) {  // 不连续
                    int32_t left = _receiveGlideWindow.left();
                    int32_t right = _receiveGlideWindow.right();
                    LOGD(TAG, "left[%d], right[%d]\n", left, right);
                    bool normal = left < right;
                    bool greater = false;
                    if ((normal && (seqNumber > estimatedSeqNumber))) {
                        greater = true;
                    }

                    if (!normal) {
                        int32_t tmpEstimatedSeqNumber = estimatedSeqNumber;
                        if (tmpEstimatedSeqNumber > left) {
                            tmpEstimatedSeqNumber = tmpEstimatedSeqNumber - UDTBasePacket::SEQUENCE_NUMBER_MAX - 1;  // 负数
                        }
                        int32_t tmpSeqNumber = seqNumber;
                        if (tmpSeqNumber > left) {
                            tmpSeqNumber = tmpSeqNumber - UDTBasePacket::SEQUENCE_NUMBER_MAX - 1;
                        }
                        if (tmpSeqNumber > tmpEstimatedSeqNumber) {
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
                        // 6.b 如果序列号小于 lrsn，将它从接收端丢失列表中移除
                        _receiveLossList.removeLossPacket(seqNumber);
                        LOGD(TAG, "removeLossPacket seqNumber[%d]\n", seqNumber);
                        greaterLrsn = false;
                    }
                }

                // 7. 更新lrsn
                if (greaterLrsn) {
                    _lrsn = seqNumber;
                    LOGD(TAG, "_lrsn[%d]\n", _lrsn);
                }
                
            } else {  // 序列号无法进入滑动窗口，就丢弃掉
                LOGW(TAG, "The seqNumber[%d] over receive glide window! drop it! current left[%d], right[%d]\n", seqNumber, _receiveGlideWindow.left(), _receiveGlideWindow.right());
            }
        } else {
            LOGE(TAG, "UDTSocket[%zd] id[%d] parse data packet error!\n", shared_from_this().get(), _socketId);
        }
    }

    void UDTSocket::changeSocketStatus(SocketStatus socketStatus) {
        if (_socketStatus != socketStatus) {
            bool needCallback = false;
            if (_socketStatus == CONNECTED || socketStatus == CONNECTED) {
                needCallback = true;
            }
            
            {
                std::unique_lock<std::mutex> lock(_sndBufMutex);
                _socketStatus = socketStatus;
                _sndCondition.notify_all();
            }
            
            if (_socketStatus == CONNECTED) {
                _setupTime = getCurrentTimeStampMicro();
                LOGW(TAG, "UDTSocket[%zd] id[%d] connection setup success! lrsn[%d]\n", shared_from_this().get(), _socketId, _lrsn);
            }
            if (needCallback && _statusCallBack) {
                _statusCallBack(_socketId, socketStatus == CONNECTED, _user);
            }
        }
    }

    // 只负责数据包发送逻辑
    void UDTSocket::sendLoop() {
        LOGW(TAG, "UDTSocket[%zd] id[%d] sendLoop running!\n", shared_from_this().get(), _socketId);
        
        while (_running) {
            uint64_t beginTime = getCurrentTimeStampMicro();

            // 1.如果发送丢失列表不为空，重传丢失列表中的第一个包，并从列表中移除，转到5
            int32_t seqNumber = -1;
            bool sendAppData = true;
            if (_sendLossList.removeFirstLossPacket(seqNumber) && seqNumber >= 0) {  // 移除成功
                auto packetPtr = _sendGlideWindow.getPacket(seqNumber);
                if (packetPtr.get()) {
                    LOGW(TAG, "UDTScoket[%zd] id[%d] Retransmission packet, seq number is [%d]\n", shared_from_this().get(), _socketId, packetPtr->seqNumber());
                    sendDataPacket(packetPtr);
                }
                else {
                    LOGW(TAG, "UDTScoket[%zd] id[%d] Retransmission packet seq number [%d] failed, because the packet is't in send glide window!\n", shared_from_this().get(), _socketId, seqNumber);
                }
                
                // 5.如果这个序列号是16n，转到2
                if (seqNumber % 16 != 0) {
                    sendAppData = false;
                }
            } else {
//                LOGD(TAG, "UDTScoket[%zd] id[%d] send loss list is empty!\n", shared_from_this().get(), _socketId);
            }

            while (_running && sendAppData) {
                // 2.todo:报文模式时，如果包存在丢失列表中时间超过应用指定的ttl生存时间，发送消息丢弃请求并且从丢失列表中移除所有相应的包，转到1
                if (_socketType == DGRAM) {
                    // todo:
                }

                std::unique_lock<std::mutex> lock(_sndBufMutex);  // 发送缓冲锁
                // 3.等待直到有应用数据需要发送，这里会出现问题，假如此时发送窗口已满，对方在等待你的重传，而你在等待对方的响应，那么就会一直等待了
                while (_running && _sendLossList.empty() && (_sndBuf.empty() || _socketStatus != CONNECTED)) {
//                    LOGD(TAG, "UDTSocket[%zd] id[%d] wait sndbuf not empty begin\n", shared_from_this().get(), _socketId);
                    _sndCondition.wait(lock);
//                    LOGD(TAG, "UDTSocket[%zd] id[%d] wait sndbuf not empty end\n", shared_from_this().get(), _socketId);
                }
                
                if (!_running) {
                    break;
                }

                std::shared_ptr<UDTDataPacket> packetPtr;
                if (!_sndBuf.empty()) {
                    packetPtr = _sndBuf.front();
                }
                if (!packetPtr.get()) {
                    break;
                }
                
                if (packetPtr->seqNumber() < 0) {  // seqNumber没有初始化的话
                    _seqNumber = GlideWindow::increaseSeqNumber(_seqNumber);  // 如果此时还未连接，那么序列号就有问题
                    packetPtr->setSeqNumber(_seqNumber);
                }

                std::unique_lock<std::mutex> lock_(_sendWindowMutex);  // 发送活动窗口锁
                // 4.b 能放入滑动窗口，打包成一个新的数据包并发送出去
                if (_sendGlideWindow.trySendPacket(packetPtr, _setupTime)) {
                    sendDataPacket(packetPtr);
                    _sndBuf.pop();
                } else if (_sendLossList.empty()) { // 4.a.如果这个未被确认过的包无法放入滑动窗口，那么等待直到 ack包到来，转到1
                    lock.unlock();  // 先解锁前面的锁
                    _waitAck = true;
                    while (_running && _waitAck && _sendLossList.empty()) {
//                        LOGD(TAG, "UDTSocket[%zd] id[%d] wait ACK begin\n", shared_from_this().get(), _socketId);
                        _sndCondition.wait(lock_);
//                        LOGD(TAG, "UDTSocket[%zd] id[%d] wait ACK end\n", shared_from_this().get(), _socketId);
                    }
                    break;
                } else {  // 如果发送丢失列表不为空，那么应该优先重传丢失列表的包
                    break;
                }

                // 5.如果这个序列号是16n，转到2
                if (seqNumber % 16 != 0) {  // todo:这步是为了什么？
                    break;
                }
            }
            
            if (!_running) {
                break;
            }

            // 6.等待 snd - t 时间，snd 是发送间隔 t 是从第一步到第五步的时长，转到1
            std::unique_lock<std::mutex> lock(_sndMutex);  // 发送定时器锁
            if (_snd > 0) {
                while (_running && (getCurrentTimeStampMicro() < (beginTime + _snd))) {
                    _sndCondition.wait_for(lock, std::chrono::microseconds(beginTime + _snd - getCurrentTimeStampMicro()));
                }
            }
        }

        LOGW(TAG, "UDTSocket[%zd] id[%d] sendLoop stoped!\n", shared_from_this().get(), _socketId);
    }

    void UDTSocket::receiveLoop() {
        // 由于这个线程比socket对象存活得久，所以需要将socket引用计数加1，延缓socket的析构
//        auto holdPtr = shared_from_this();
        LOGW(TAG, "UDTSocket[%zd] id[%d] receiveLoop running!\n", shared_from_this().get(), _socketId);
        
        while (_running) {
            // 1.查询系统时间检查ack、nak、exp定时器是否超时，如果任何一个超时，处理超时时间并重置相应的时间变量，对于ack，还要检查ack包间隔
            if (_socketStatus == UNCONNECTED || _socketStatus == CONNECTING) {  // 未连接或者正在连接
                synTask();
            } else if (_socketStatus != SHUTDOWN) {
                ackTask();
                nakTask();
                if (expTask()) {
                    break;
                }
            }

            // 2.开始有限制的接收udp，如果没有包到达，转到1
            std::shared_ptr<UdpData> udpDataPtr = tryGetReceivedPacket();
            if (udpDataPtr.get()) {
                // 2.1 重置连续超时次数为1，如果没有未响应的数据，或者这是一个ack或nak控制包，重置exp定时器
                _expCount = 1;
                _expCountResetTime = getCurrentTimeStampMicro();
                UDTControlPacket::ControlPacketType controlPacketType = UDTControlPacket::ControlPacketType::UNKNOW;
                bool isControlPacket = UDTControlPacket::decodeControlTypeStatic(udpDataPtr->data(),
                                                                                 udpDataPtr->length(),
                                                                                 controlPacketType);
                if (_sendGlideWindow.empty() || (isControlPacket && (controlPacketType ==
                                                                     UDTControlPacket::ControlPacketType::ACK ||
                                                                     controlPacketType ==
                                                                     UDTControlPacket::ControlPacketType::NAK))) {
                    resetExpTimer(_expCountResetTime);
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

        LOGW(TAG, "UDTSocket[%zd] id[%d] receiveLoop stoped!\n", shared_from_this().get(), _socketId);
    }

    void UDTSocket::tryConnectionSetup() {
        if ((_socketStatus == UNCONNECTED || _socketStatus == CONNECTING) &&
            _connectionType != SERVER) {  // 未连接状态，需要尝试连接，服务端模式不需要发送连接请求
            sendSynPacket(_connectionType == RENDEZVOUS ? RENDEZVOUS_REQUEST : CLIENT_REQUEST);
            changeSocketStatus(CONNECTING);
        }
    }

    void UDTSocket::sendDataPacket(std::shared_ptr<UDTDataPacket> dataPacketPtr) {
        LOGD(TAG, "UDTSocket[%zd] id[%d] sendDataPacket, seq number is [%d]\n", shared_from_this().get(), _socketId, dataPacketPtr->seqNumber());
        auto sharedPtr = _multiplexerPtr.lock();
        if (sharedPtr.get()) {
            char tmpBuf[_maxPacketSize];
            uint64_t currentTime = getCurrentTimeStampMicro();
            dataPacketPtr->setDstSocketId(_dstSocketId);
            int len = dataPacketPtr->encode(tmpBuf, _maxPacketSize);
            if (len > 0) {
                sharedPtr->sendData(_remoteAddr, _remotePort, tmpBuf, (size_t) len);
            }
        }
    }

    void UDTSocket::resetExpTimer(uint64_t time) {
        _nakTime = time;
    }

}
