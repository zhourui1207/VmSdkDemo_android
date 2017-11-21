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
#include "../util/UdpData.h"
#include <queue>
#include <condition_variable>
#include "ReceiveGlideWindow.h"
#include "ReceiveLossList.h"
#include "SendGlideWindow.h"
#include "AckHistoryWindow.h"
#include "MedianUtil.h"
#include "packet/UDTAckPacket.h"
#include "packet/UDTNakPacket.h"
#include "packet/UDTAck2Packet.h"
#include "packet/UDTMDRPacket.h"
#include "packet/UDTShutdownPacket.h"

namespace Dream {

    class UDTMultiplexer;

    class UDTSocket : public std::enable_shared_from_this<UDTSocket> {

    public:
        const static uint32_t UDT_VERSION = 4;  // 当前版本号4

    private:
        const char *TAG = "UDTSocket";
        const static uint32_t INIT_RTT_US = 100000;  // 0.1初始值秒
        const static uint32_t INIT_BUFFER_SIZE = 16;

        // The initial congestion window size is 16 packets and the initial
        // inter-packet interval is 0.
        const static uint32_t SYN_TIME_US = 10000;  // 0.01sec 10000微秒

        const static uint32_t EXP_PERIOD_MIN = 500000;  // exp定时器的最小阀值 0.5秒

        const static uint32_t SYN_PERIOD_US = 2000000;  // 2秒

//        const static uint16_t RTT_SAMPLE_NUM = 5;  // tts样本数

        const static std::size_t BUFFER_SIZE_DEFAULT = 100000;  // 默认缓存队列大小

        const static uint32_t HISTORY_WINDOW_SIZE = 17;

        const static uint32_t COOKIE_SECRET_KEY = 19891207;
        
        const static std::size_t DGRAM_PACKET_MAX_SIZE_DEFAULT = 10240;  // 报文包最大长度限制为10M，用户可以自行设置

        enum ConnectionType {
            RENDEZVOUS_REQUEST = 0,
            CLIENT_REQUEST = 1,
            FIRST_RESPONSE = -1,
            SECOND_RESPONSE = -2
        };

    public:
        virtual ~UDTSocket();

        bool postData(const char *pBuf, std::size_t len);

        void shutdown();  // 关闭

        std::string getRemoteAddr() const;

        uint16_t getRemotePort() const;
        
        uint32_t getDstSocketId() const;

    private:
        UDTSocket(SocketType socketType, ConnectionMode connectionType,
                  std::shared_ptr<UDTMultiplexer> multiplexePtr, uint32_t socketId,
                  const std::string &remoteAddr, uint16_t remotePort,
                  fUDTDataCallBack dataCallBack, fUDTConnectStatusCallBack statusCallBack,
                  void *pUser, uint32_t dstSocketId = 0, uint32_t maxPacketSize = DEFAULT_MTU,
                  uint32_t sendWindowSize = DEFAULT_WINDOW_SIZE_MAX,
                  uint32_t recvWindowSize = DEFAULT_WINDOW_SIZE_MAX,
                  uint32_t synPeriod = SYN_PERIOD_US,
                  std::size_t sndBufSize = BUFFER_SIZE_DEFAULT,
                  std::size_t recBufSize = BUFFER_SIZE_DEFAULT,
                  std::size_t dgramPacketMaxSize = DGRAM_PACKET_MAX_SIZE_DEFAULT);

        static bool isConnectionRequest(const char* pBuf, std::size_t len);
        
        static uint32_t socketId(const char* pBuf, std::size_t len);
        
        void setup();  // 连接
        
        void shutdownByReceiveLoop();

        void sendShutdownNotify();

        void changeSocketStatus(SocketStatus socketStatus);

        void sendLoop();

        void receiveLoop();

        void tryConnectionSetup();

        void sendDataPacket(std::shared_ptr<UDTDataPacket> dataPacketPtr);

        void
        receiveData(const std::string &remote_address, uint16_t remote_port, const char *pBuf,
                    std::size_t len);

        std::shared_ptr<UdpData> tryGetReceivedPacket();

        // 处理数据包
        void handleDataPacket(std::shared_ptr<UdpData> udpDataPtr);

        // 处理控制包
        void handleControlPacket(std::shared_ptr<UdpData> udpDataPtr,
                                 const UDTControlPacket::ControlPacketType &controlPacketType);

        void handleAckPacket(const UDTAckPacket &packet);

        void handleNakPacket(const UDTNakPacket &packet);

        void handleAck2Packet(const UDTAck2Packet &packet);

        void handleMDRPacket(const UDTMDRPacket &packet);

        void handleSynPacket(const UDTSynPacket &packet);

        void handleShutdownPacket(const UDTShutdownPacket &packet);

        void initSocket(const UDTSynPacket &packet);

        void addTimestampAndDstSocketId(UDTBasePacket& basePacket);

        void sendSynPacket(int32_t connectionType);

        void sendKeepAlivePacket();

        void sendAckPacket(int32_t seqNumber, bool byAckTimer);

        void sendNakPacket(const std::vector<int32_t> &seqNumberList);

        void sendAck2Packet(int32_t ackSeqNumber);

        void sendShutdownPacket();

        void sendMDRPacket();

        void shutdownWithoutRemove();

        bool handleRevice();

        void synTask();

        void ackTask();

        void nakTask();

        bool expTask();

        void sndTask();

        void resetExpTimer(uint64_t time);

//        NAK is used to trigger a negative acknowledgement (NAK). Its period
//        is dynamically updated to 4 * RTT_+ RTTVar + SYN, where RTTVar is the
//        variance of RTT samples.
        void updateRtt(int32_t rttUs);

        void updateRttVariance(int32_t rttUs);

        void updateAckAndNakPeriod();

        void updatePacketArrivalRate(int32_t rate);

        void updateEstimatedLinkCapacity(int32_t capacity);


//        EXP is used to trigger data packets retransmission and maintain
//                connection status. Its period is dynamically updated to N * (4 * RTT
//        + RTTVar + SYN), where N is the number of continuous timeouts. To
//                avoid unnecessary timeout, a minimum threshold (e.g., 0.5 second)
//        should be used in the implementation.
        uint32_t getExpPeriod();

        bool synTimeout();

        bool ackTimeout();

        bool nakTimeout();

        bool expTimeout();

        bool sndTimeout();

        bool addressIsEquals(const uint32_t *a, const uint32_t *b);

        void generateCookie();
        
        void reliablePacketCallback(std::shared_ptr<UDTDataPacket> udtDataPacketPtr);

        friend class UDTMultiplexer;

    private:
        SocketStatus _socketStatus;
        const SocketType _socketType;
        int32_t _seqNumber;
        uint32_t _maxPacketSize;
        uint32_t _sendWindowSize;
        uint32_t _recvWindowSize;
        const ConnectionMode _connectionType;
        uint32_t _cookie;
        std::weak_ptr<UDTMultiplexer> _multiplexerPtr;
        const uint32_t _socketId;  // 这是自己的socketid，跟dstsocketid不同
        uint32_t _dstSocketId;  // 目的的socketid
        std::string _remoteAddr;
        uint16_t _remotePort;
        fUDTDataCallBack _dataCallBack;
        fUDTConnectStatusCallBack _statusCallBack;
        void *_user;
        uint32_t _remoteAddrBytes[4];
        uint32_t _localAddrBytes[4];

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

        int32_t _largestACK2SeqNumber;
        MedianUtil _medianUtil;
        AckHistoryWindow _ackHistoryWindow;
        uint64_t _pkt[HISTORY_WINDOW_SIZE];  // 数据包到达的时间
        int _pktIndex;
        int64_t _packetPair[HISTORY_WINDOW_SIZE];  // 数据包到达时间间隔
        int _packetPairIndex;

        std::mutex _mutex;

        int32_t _ackSeqNumber;
//        m_iRTT = 10 * m_iSYNInterval;  m_iSYNInterval 初始值是0.01秒
//        m_iRTTVar = m_iRTT >> 1;
        int32_t _rttUs;  // rtt平均值
        int32_t _rttVariance;  // rtt样本的方差
        uint32_t _bufferSize;
//        uint32_t _receiveRate;
//        uint32_t _linkCapacity;

        uint32_t _packetArrivalRate;
        uint32_t _estimatedLinkCapacity;

//        int32_t _rttSamples[RTT_SAMPLE_NUM];
//        int16_t _rttSampleIndex;
        int32_t _snd;  // 发送间隔 这个变量关系到发送速度 文档上的建议，初始化时为0，后面在流量控制时更新值
        std::mutex _sndMutex;

        std::mutex _sendWindowMutex;
        bool _waitAck;

        int32_t _lrsn;  // 最大接收包序列号

        uint32_t _expCount;  // 连续超时次数
        uint64_t _expCountResetTime;  // 重置超时次数的时间

        // 四个定时器
        uint64_t _synTime;  // 发送syn时间
        uint64_t _ackTime;  // 响应定时器，周期不能大于 SYN TIME
        uint64_t _nakTime;  // 丢包响应定时器
        uint64_t _expTime;  // 重发，维持连接状态定时器
        uint64_t _sndTime;  // 发送

        uint32_t _synPeriod;
        uint32_t _ackPeriod;
        uint32_t _nakPeriod;
        uint32_t _expPeriod;

        // 缓存
        bool _handlePacket;
        bool _handling;

        uint64_t _setupTime;
        
        char* _dgramBuf;
        const std::size_t _dgramBufMaxLen;
        std::size_t _dgramBufLen;
        
        bool _slowStartPhase;
        int32_t _LastACK;
        int32_t _LastDecPeriod;
        int32_t _AvgNAKNum;
        int32_t _NAKCount;
        int32_t _DecCount;
        int32_t _LastDecSeq;
        int32_t _DecRandom;
    };

}

#endif //VMSDKDEMO_ANDROID_UDTSESSION_H
