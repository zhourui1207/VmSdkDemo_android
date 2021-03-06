/*
 * UasClient.h
 *
 *  Created on: 2016年9月30日
 *      Author: zhourui
 *      这个是与uas服务交互通信的客户端线程，提供给使用者与uas交互的接口
 */

#ifndef UASCLIENT_H_
#define UASCLIENT_H_

#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>

#include "../util/TcpClient.h"
#include "../util/Timer.h"
#include "packet/AlarmPacket.h"
#include "packet/ChannelPacket.h"
#include "packet/DepTreePacket.h"
#include "packet/RecordPacket.h"

namespace Dream {

// 可以配置线程池最大线程数量来提高uas客户端的并发处理能力
    class UasClient : public TcpClient<MsgPacket> {
    public:
        static const uint64_t TIMEOUT_DEFULT = 5000; // 默认超时5秒
        static const uint64_t HEARTBEAT_INTERVAL = 10000;  // 默认心跳间隔5秒

    private:
        class PlaybackSession {
        public:
            PlaybackSession() = delete;

            PlaybackSession(bool isCenter, const std::string &fdId, int channelId)
                    : _isCenter(isCenter), _fdId(fdId), _channelId(channelId) {

            }

            bool IsCenter() {
                return _isCenter;
            }

            std::string FdId() {
                return _fdId;
            }

            int ChannelId() {
                return _channelId;
            }

        private:
            bool _isCenter;
            std::string _fdId;
            int _channelId;
        };

        // 通过一个session管理器来管理session，避免直接操作数据引起的锁问题
        class PlaybackSeesionManager {
        public:
            PlaybackSeesionManager() = default;

            bool AddSession(unsigned monitorId, bool isCenter, const std::string &fdId,
                            int channelId) {
                std::unique_lock<std::mutex> lock(_mutex);

                auto sessionIt = _playbackSessionMap.find(monitorId);
                if (sessionIt != _playbackSessionMap.end()) {
                    return false;
                }

                _playbackSessionMap.emplace(std::make_pair(monitorId,
                                                           std::make_shared<PlaybackSession>(
                                                                   isCenter, fdId, channelId)));
                return true;
            }

            void removeSession(unsigned monitorId) {
                std::unique_lock<std::mutex> lock(_mutex);

                auto sessionIt = _playbackSessionMap.find(monitorId);
                if (sessionIt != _playbackSessionMap.end()) {
                    _playbackSessionMap.erase(sessionIt);
                }
            }

            std::shared_ptr<PlaybackSession> getSession(unsigned monitorId) {
                std::shared_ptr<PlaybackSession> retSession;

                std::unique_lock<std::mutex> lock(_mutex);
                auto sessionIt = _playbackSessionMap.find(monitorId);
                if (sessionIt != _playbackSessionMap.end()) {
                    retSession = sessionIt->second;
                }
                return retSession;
            }

        private:
            std::mutex _mutex;
            std::map<unsigned, std::shared_ptr<PlaybackSession>> _playbackSessionMap;  // 录像回放时的session信息
        };

    public:
        UasClient() = delete;

        UasClient(ThreadPool &pool, const std::string &address, unsigned port,
                  uint64_t heartbeatInterval = HEARTBEAT_INTERVAL, uint64_t timeout =
        TIMEOUT_DEFULT) :
                TcpClient<MsgPacket>(pool, address, port), _running(false), _seqNumber(0),
                _monitorSessionId(0), _receiveSeqNumber(0), _heartbeatInterval(heartbeatInterval),
                _timeout(timeout), _alarmListener(nullptr) {

        }

        virtual ~UasClient() {
            onClose();
        }

        // 启动
        virtual bool startUp() override {
            if (!_running) {
                _running = true;
                return TcpClient<MsgPacket>::startUp();
            } else {
                return false;
            }
        }

        // 关闭
        virtual void shutDown() override {
            if (_running) {
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    _running = false;
                    _condition.notify_all();
                }
                TcpClient<MsgPacket>::shutDown();
            }
        }

        // 子类用这个函数解析包,返回一个MsgPacket智能指针,不需要delete，底层会自动删除内存
        virtual std::shared_ptr<MsgPacket> newPacketPtr(unsigned msgType) override;

        // 子类继承这个用来接收包
        virtual void receive(const std::shared_ptr<MsgPacket> &packetPtr) override;

        // 连接成功
        virtual void onConnect() override;

        // 连接断开
        virtual void onClose() override;

        // 封装的操作接口开始------------------------------------------------
        // 登录
        unsigned login(const std::string &loginName, const std::string &loginPwd);

        // 登出
        void logout();

        // 获取行政树
        unsigned getDepTrees(DepTreeList &depTrees);

        // 获取通道列表
        unsigned getChannels(int depId, ChannelList &channels);

        // 查询录像列表
        unsigned getRecords(const std::string &fdId, int channelId,
                            unsigned beginTime, unsigned endTime, bool isCenter,
                            RecordList &records);

        // 查询报警信息
        unsigned getAlarms(const std::string &fdId, int channelId,
                           unsigned channelBigType, const std::string &beginTime,
                           const std::string &endTime, AlarmList &alarms);

        // 打开实时码流地址
        unsigned openRealplayStream(const std::string &fdId, int channelId,
                                    bool isSub, unsigned &monitorId, std::string &videoAddr,
                                    unsigned &videoPort,
                                    std::string &audioAddr, unsigned &audioPort);

        // 停止码流地址
        void closeRealplayStream(unsigned monitorId);

        // 打开回放流地址
        unsigned openPlaybackStream(const std::string &fdId, int channelId,
                                    bool isCenter, unsigned beginTime, unsigned endTime,
                                    unsigned &monitorId,
                                    std::string &videoAddr, unsigned &videoPort,
                                    std::string &audioAddr,
                                    unsigned &audioPort);

        // 停止回放流
        void closePlaybackStream(unsigned monitorId);

        // 控制回放
        unsigned controlPlayback(unsigned monitorId, unsigned controlId,
                                 const std::string &action, const std::string &param);

        // 控制指令
        void sendCmd(const std::string &fdId, int channelId, unsigned controlType,
                     unsigned param1, unsigned param2);

        // 设置报警侦听
        void setAlarmListener(
                std::function<
                        void(const std::string &fdId, int channelId, unsigned alarmType,
                             unsigned param1, unsigned param2)> alarmListener) {
            _alarmListener = alarmListener;
        }

        // 取消报警侦听
        void cancelAlarmListener() {
            _alarmListener = nullptr;
        }

        // 封装的操作接口结束------------------------------------------------

    private:
        // 心跳
        void heartbeat();

        // 生成序列号
        unsigned generateSeqNumber() {
            _seqNumber = (_seqNumber % UINT32_MAX) + 1;
            return _seqNumber;
        }

        // 生成预览sessionid
        unsigned generateMonitorSessionId() {
            std::lock_guard<std::mutex> lock(_mutex);
            _monitorSessionId = (_monitorSessionId % UINT32_MAX) + 1;
            return _monitorSessionId;
        }

        // 处理响应
        void handleResp(const std::shared_ptr<MsgPacket> &packetPtr);

        // 处理报警消息推送
        void handleAlarmNotity(const std::shared_ptr<MsgPacket> &packetPtr);

        // 等待响应包，如果返回false，表示发送失败；如果返回true，但是respPacket里的包为空，则是返回超时
        bool sendAndWaitRespPacket(MsgPacket &msgPacket,
                                   std::shared_ptr<MsgPacket> &respPacket);

    private:
        const char* TAG = "UasClient";
        bool _running;
        // 序列号
        unsigned _seqNumber;
        // 预览sessionid
        unsigned _monitorSessionId;
        // 接收到的序列号
        unsigned _receiveSeqNumber;
        uint64_t _heartbeatInterval;
        uint64_t _timeout;
        std::mutex _mutex;
        std::condition_variable _condition;
        std::map<unsigned, std::shared_ptr<MsgPacket>> _receivePackets; // 接收到的包
        std::shared_ptr<Timer> _timerPtr;  // 心跳定时器智能指针
        PlaybackSeesionManager _playbackSessionManager;  // session管理器

        std::function<
                void(const std::string &fdId, int channelId, unsigned alarmType,
                     unsigned param1, unsigned param2)> _alarmListener;
    };

} /* namespace Dream */

#endif /* UASCLIENT_H_ */
