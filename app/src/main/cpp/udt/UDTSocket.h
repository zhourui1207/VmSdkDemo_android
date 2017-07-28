//
// Created by zhou rui on 2017/7/13.
//

#ifndef DREAM_UDTSOCKET_H
#define DREAM_UDTSOCKET_H

#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include "UDTType.h"
#include "packet/UDTSynPacket.h"
#include "../util/ThreadPool.h"
#include "../util/UdpData.h"
#include "ReceiveGlideWindow.h"
#include "ReceiveLossList.h"
#include "SendGlideWindow.h"

namespace Dream {

    class UDTMultiplexer;

    class UDTSocket {

    public:
        const static uint32_t UDT_VERSION = 4;  // 当前版本号4

    private:
        const char *TAG = "UDTSocket";
        const static uint32_t INIT_RTT_US = 100000;  // 0.1初始值秒
        const static uint32_t INIT_BUFFER_SIZE = 16;

        const static uint32_t SYN_TIME_US = 10000;  // 0.01sec 10000微秒
        const static uint32_t EXP_PERIOD_MIN = 500000;  // exp定时器的最小阀值 0.5秒

        const static uint16_t RTT_SAMPLE_NUM = 5;  // tts样本数

        const static std::size_t BUFFER_SIZE_DEFAULT = 5000;  // 默认缓存队列大小

        const static uint32_t HISTORY_WINDOW_SIZE = 16;

    public:
        virtual ~UDTSocket();

        bool postData(const char *pBuf, std::size_t len);

        void shutdown();  // 关闭

        std::string getRemoteAddr() const;

        unsigned short getRemotePort() const;

    private:
        UDTSocket(SocketType socketType, ConnectionType connectionType,
                  std::shared_ptr<UDTMultiplexer> multiplexePtr, uint32_t socketId,
                  const std::string &remoteAddr, uint16_t remotePort,
                  fUDTDataCallBack dataCallBack, fUDTConnectStatusCallBack statusCallBack,
                  void *pUser, uint32_t maxPacketSize = DEFAULT_MTU,
                  uint32_t maxWindowSize = DEFAULT_WINDOW_SIZE_MAX,
                  std::size_t sndBufSize = BUFFER_SIZE_DEFAULT,
                  std::size_t recBufSize = BUFFER_SIZE_DEFAULT);

        void setup();  // 连接

        void changeSocketStatus(SocketStatus socketStatus);

        void sendLoop();

        void receiveLoop();

        void checkSocketStatus();

        void sendDataPacket(std::shared_ptr<UDTDataPacket> dataPacketPtr);

        void
        receiveData(const std::string &remote_address, uint16_t remote_port, const char *pBuf,
                    std::size_t len);

        std::shared_ptr<UdpData> tryGetReceivedPacket();

        // 处理控制包
        void handleControlPacket(std::shared_ptr<UdpData> udpDataPtr,
                                 const UDTControlPacket::ControlPacketType &controlPacketType);

        // 处理数据包
        void handleDataPacket(std::shared_ptr<UdpData> udpDataPtr);

        void sendSynPacket();

        void handleSynPacket(const UDTSynPacket &packet);

        void sendKeepAlivePacket();

        void sendAckPacket(int32_t seqNumber);

        void sendNakPacket(const std::vector<int32_t > &seqNumberList);

        void sendAck2Packet(int32_t ackSeqNumber);

        void sendShutdownPacket();

        void sendMDRPacket();

        void shutdownWithoutRemove();

        bool handleRevice();

        void ackTask();

        void nakTask();

        void expTask();

        void sndTask();


//        NAK is used to trigger a negative acknowledgement (NAK). Its period
//        is dynamically updated to 4 * RTT_+ RTTVar + SYN, where RTTVar is the
//        variance of RTT samples.

        void updateRttVariance();

        uint32_t getNakPeriod();


//        EXP is used to trigger data packets retransmission and maintain
//                connection status. Its period is dynamically updated to N * (4 * RTT
//        + RTTVar + SYN), where N is the number of continuous timeouts. To
//                avoid unnecessary timeout, a minimum threshold (e.g., 0.5 second)
//        should be used in the implementation.

        uint32_t getExpPeriod();

        bool ackTimeout();

        bool nakTimeout();

        bool expTimeout();

        bool sndTimeout();

        friend class UDTMultiplexer;

    private:
        SocketStatus _socketStatus;
        const SocketType _socketType;
        int32_t _seqNumber;
        const std::size_t _maxPacketSize;
        std::size_t _maxWindowSize;
        const ConnectionType _connectionType;
        uint32_t _cookie;
        std::shared_ptr<UDTMultiplexer> _multiplexerPtr;
        const uint32_t _socketId;
        std::string _remoteAddr;
        uint16_t _remotePort;
        fUDTDataCallBack _dataCallBack;
        fUDTConnectStatusCallBack _statusCallBack;
        void *_user;
        uint32_t _addrBytes[4];

        bool _running;

        std::mutex _sndBufMutex;
        std::condition_variable _sndCondition;
        std::queue<std::shared_ptr<UDTDataPacket>> _sndBuf;  // 发送缓存
        std::size_t _sndBufSize;
        std::unique_ptr<std::thread> _sendThreadPtr;  // 发送线程
        SendGlideWindow _sendGlideWindow;  // 发送端滑动窗口
        SendLossList _sendLossList;  // 发送端丢失列表

        std::mutex _recBufMutex;
        std::condition_variable _recCondition;
        std::queue<std::shared_ptr<UdpData>> _recBuf;  // 接收缓存
        std::size_t _recBufSize;
        std::unique_ptr<std::thread> _receiveThreadPtr;  // 接收线程
        ReceiveGlideWindow _receiveGlideWindow;  // 接收端滑动窗口
        ReceiveLossList _receiveLossList;  // 接收端丢失列表

        uint64_t _pkt[HISTORY_WINDOW_SIZE];  // 数据包到达的时间
        int _pktIndex;
        int64_t _packetPair[HISTORY_WINDOW_SIZE];  // 数据包到达时间间隔
        int _packetPairIndex;

        std::mutex _mutex;

        bool _firstAck;
        int32_t _ackSeqNumber;
        uint32_t _rttUs;  // rtt平均值
        uint32_t _rttVariance;  // rtt样本的方差
        uint32_t _bufferSize;
        uint32_t _receiveRate;
        uint32_t _linkCapacity;
        uint32_t _rttSamples[RTT_SAMPLE_NUM];
        uint16_t _rttSampleIndex;
        uint32_t _snd;  // 发送间隔
        std::mutex _sndMutex;

        std::mutex _sendWindowMutex;
        bool waitAck;

        int32_t _lrsn;  // 最大接收包序列号
        uint32_t _expCount;  // 连续超时次数

        // 四个定时器
        uint64_t _ackTime;  // 响应定时器，周期不能大于 SYN TIME
        uint64_t _nakTime;  // 丢包响应定时器
        uint64_t _expTime;  // 重发，维持连接状态定时器
        uint64_t _sndTime;  // 发送

        // 缓存
        bool _handlePacket;
        bool _handling;
        ThreadPool _threadPool;

        uint64_t _setupTime;

        void resetExpTimer();
    };

}

#endif //VMSDKDEMO_ANDROID_UDTSESSION_H
