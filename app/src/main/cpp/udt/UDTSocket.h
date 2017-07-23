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

    public:
        virtual ~UDTSocket();

        void shutdown();  // 关闭

        std::string getRemoteAddr() {
            return _remoteAddr;
        }

        unsigned short getRemotePort() {
            return _remotePort;
        }

        void changeSocketStatus(SocketStatus socketStatus);

    private:
        UDTSocket(SocketType socketType, ConnectionType connectionType,
                  std::shared_ptr<UDTMultiplexer> multiplexePtr, uint32_t socketId,
                  const std::string &remoteAddr, uint16_t remotePort,
                  fUDTDataCallBack dataCallBack, fUDTConnectStatusCallBack statusCallBack,
                  void *pUser, uint32_t maxPacketSize = DEFAULT_MTU,
                  uint32_t maxWindowSize = DEFAULT_WINDOW_SIZE_MAX);

        void setup();  // 连接

        void run();

        void checkSocketStatus();

        void
        receiveData(const std::string &remote_address, uint16_t remote_port, const char *pBuf,
                    std::size_t len);

        void handleData(std::shared_ptr<UdpData> udpDataPtr);

        void sendSynPacket();

        void handleSynPacket(const UDTSynPacket& packet);

        void sendKeepAlivePacket();

        void sendAckPacket(unsigned seqNumber);

        void sendNakPacket();

        void sendAck2Packet(unsigned ackSeqNumber);

        void sendShutdownPacket();

        void sendMDRPacket();

        void shutdownWithoutRemove();

        void handleRevice();

        void ackTask();

        void nakTask();

        void expTask();

        void sndTask();


//        NAK is used to trigger a negative acknowledgement (NAK). Its period
//        is dynamically updated to 4 * RTT_+ RTTVar + SYN, where RTTVar is the
//        variance of RTT samples.

        void updateRttVariance() {  // 获取rtt样本方差
            uint32_t sum = 0;
            for (int i = 0; i < RTT_SAMPLE_NUM; ++i) {
                uint32_t offset = _rttSamples[i] - _rttUs;
                sum += (offset * offset);
            }
            _rttVariance = sum / RTT_SAMPLE_NUM;
        }

        uint32_t getNakPeriod() {
            return 4 * _rttUs + _rttVariance + SYN_TIME_US;
        }


//        EXP is used to trigger data packets retransmission and maintain
//                connection status. Its period is dynamically updated to N * (4 * RTT
//        + RTTVar + SYN), where N is the number of continuous timeouts. To
//                avoid unnecessary timeout, a minimum threshold (e.g., 0.5 second)
//        should be used in the implementation.

        uint32_t getExpPeriod() {
            unsigned period = _timeoutNumber * getNakPeriod();
            return period < EXP_PERIOD_MIN ? EXP_PERIOD_MIN : period;
        }

        bool ackTimeout() {
            uint64_t currentTime = getCurrentTimeStampMicro();
            if (_ackTime > 0 && (_ackTime + SYN_TIME_US > currentTime)) {  // 没超时
                return false;
            } else {
                _ackTime = currentTime;
                return true;
            }
        }

        bool nakTimeout() {
            uint64_t currentTime = getCurrentTimeStampMicro();
            if (_nakTime > 0 && (_nakTime + getNakPeriod() > currentTime)) {  // 没超时
                return false;
            } else {
                _nakTime = currentTime;
                return true;
            }
        }

        bool expTimeout() {
            uint64_t currentTime = getCurrentTimeStampMicro();
            if (_expTime > 0 && (_expTime + getExpPeriod() > currentTime)) {  // 没超时
                return false;
            } else {
                _expTime = currentTime;
                return true;
            }
        }

        bool sndTimeout() {
            uint64_t currentTime = getCurrentTimeStampMicro();
            if (_sndTime > 0 && (_sndTime + _stp > currentTime)) {  // 没超时
                return false;
            } else {
                _sndTime = currentTime;
                return true;
            }
        }

        friend class UDTMultiplexer;

    private:
        SocketStatus _socketStatus;
        SocketType _socketType;
        int32_t _seqNumber;
        std::size_t _maxPacketSize;
        std::size_t _maxWindowSize;
        ConnectionType _connectionType;
        uint32_t _cookie;
        std::shared_ptr<UDTMultiplexer> _multiplexerPtr;
        uint32_t _socketId;
        std::string _remoteAddr;
        uint16_t _remotePort;
        fUDTDataCallBack _dataCallBack;
        fUDTConnectStatusCallBack _statusCallBack;
        void *_user;
        uint32_t _addrBytes[4];

        bool _running;
        std::unique_ptr<std::thread> _threadPtr;
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
        uint32_t _stp;  // 发送间隔


        int32_t _lossedSeqNumber[INIT_BUFFER_SIZE];  // 丢失包信息
        std::size_t _lossedSize;  // 丢失包个数

        int32_t _lrsn;  // 最大接收包序列号
        uint32_t _timeoutNumber;  // 连续超时次数

        // 四个定时器
        uint64_t _ackTime;  // 响应定时器，周期不能大于 SYN TIME
        uint64_t _nakTime;  // 丢包响应定时器
        uint64_t _expTime;  // 重发，维持连接状态定时器
        uint64_t _sndTime;  // 发送

        // 缓存
        bool _handlePacket;
        bool _handling;
        std::queue<std::shared_ptr<UdpData>> _recBuf;  // 接收缓存
        ThreadPool _threadPool;
    };

}

#endif //VMSDKDEMO_ANDROID_UDTSESSION_H
