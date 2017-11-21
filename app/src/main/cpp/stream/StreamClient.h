/*
 * StreamClient.h
 *
 *  Created on: 2016年10月13日
 *      Author: zhourui
 *      流客户端，一般用于取流使用
 */

#ifndef DREAM_STREAMCLIENT_H_
#define DREAM_STREAMCLIENT_H_

#include <memory>
#include <mutex>

#include "../util/StreamData.h"
#include "../util/StreamPacket.h"
#include "../util/TcpClient.h"
#include "../util/ThreadPool.h"
#include "../util/Timer.h"

namespace Dream {

// 通常情况下是需要码流按顺序接收的，所以这里内部使用最大线程数为1的线程池
    class StreamClient : public TcpClient<StreamPacket> {
    public:
        static const uint64_t HEARTBEAT_INTERVAL = 5000;  // 默认心跳间隔5秒

    public:
        StreamClient() = delete;

        StreamClient(const std::string &address, unsigned short port,
                     std::function<void(
                             const std::shared_ptr<StreamData> &streamDataPtr)> streamCallback,
                     const std::string &monitorId, const std::string &deviceId,
                     int playType, int clientType, uint64_t heartbeatInterval = HEARTBEAT_INTERVAL) :
                TcpClient<StreamPacket>(_pool, address, port), _pool(1, 1), _streamCallback(
                streamCallback), _monitorId(monitorId), _deviceId(deviceId), _playType(playType),
                _clientType(clientType), _heartbeatInterval(heartbeatInterval), _auth(false) {
        }

        virtual ~StreamClient() {

        };

        // 启动
        virtual bool startUp() override {
            _pool.start();
            return TcpClient<StreamPacket>::startUp();
        }

        // 关闭
        virtual void shutDown() override {
            _pool.stop();
            TcpClient<StreamPacket>::shutDown();
        }

        // 子类用这个函数解析包,返回一个MsgPacket智能指针,不需要delete，底层会自动删除内存
        virtual std::shared_ptr<StreamPacket> newPacketPtr(unsigned msgType) override;

        // 子类继承这个用来接收包
        virtual void receive(const std::shared_ptr<StreamPacket> &packetPtr)
                override;

        // 连接成功
        virtual void onConnect() override;

        // 连接断开
        virtual void onClose() override;

        void cancelStreamListener() {
            _streamCallback = nullptr;
        }

    private:
        // 心跳
        void heartbeat();

        // 鉴权
        void auth();

    private:
        ThreadPool _pool;
        std::function<void(
                const std::shared_ptr<StreamData> &streamDataPtr)> _streamCallback; // 码流回调，由于使用了线程池异步，所以使用智能指针来管理内存释放比较合理

        std::mutex _mutex;
        std::shared_ptr<Timer> _timerPtr;  // 心跳定时器智能指针
        std::string _monitorId;
        std::string _deviceId;
        int _playType;
        int _clientType;
        uint64_t _heartbeatInterval;  // 心跳保活间隔
        bool _auth;
    };

} /* namespace Dream */

#endif /* STREAMCLIENT_H_ */
