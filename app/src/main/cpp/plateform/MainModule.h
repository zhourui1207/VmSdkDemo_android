/*
 * MainModule.h
 *
 *  Created on: 2016年10月18日
 *      Author: zhourui
 *      平台sdk的主模块
 */

#ifndef MAINMODULE_H_
#define MAINMODULE_H_

#include <atomic>
#include <map>
#include <memory>

#include "../stream/StreamClient.h"
#include "../uas/UasClient.h"
#include "../util/ThreadPool.h"
#include "StreamSessionManager.h"
#include "VmType.h"

namespace Dream {

class MainModule {
private:
  static const uint64_t WAIT_CONNECT_UAS_DEFAULT_TIME = 2000; // 等待连接uas成功的默认时间为2秒

public:
  MainModule() = delete;

  // maxThreadCount：最大线程数量，以传入cpu最大线程数量为最佳，能够极大增强程序并发能力
  MainModule(unsigned maxThreadCount) :
      _pool(1, maxThreadCount), _uasConnected(false), _uasConnectStatusCallback(
          nullptr), _realAlarmCallback(nullptr) {
    _pool.start();
  }
  virtual ~MainModule() {
    _pool.stop();
  }

public:
  // 封装的操作接口开始------------------------------------------------
  // 设置uas状态回调
  void setUasConnectStatusCallback(
      fUasConnectStatusCallBack uasConnectStatusCallback) {
    _uasConnectStatusCallback = uasConnectStatusCallback;
  }

  // 取消uas状态回到
  void cancelUasConnectStatusCallback() {
    _uasConnectStatusCallback = nullptr;
  }

  // 尝试连接服务器，异步连接，返回并不代表已经连接成功
  unsigned connect(const std::string& serverAddr, unsigned port);

  // 断开服务器
  void disconnect();

  // 登录
  unsigned login(const std::string& loginName, const std::string& loginPwd);

  // 登出
  void logout();

  // 获取行政树
  unsigned getDepTrees(DepTreeList& depTrees);

  // 获取通道列表
  unsigned getChannels(int depId, ChannelList& channels);

  // 查询录像列表
  unsigned getRecords(const std::string& fdId, int channelId,
      unsigned beginTime, unsigned endTime, bool isCenter, RecordList& records);

  // 查询报警信息
  unsigned getAlarms(const std::string& fdId, int channelId, int channelBigType,
      const std::string& beginTime, const std::string& endTime,
      AlarmList& alarms);

  // 打开实时码流并获取地址
  unsigned openRealplayStream(const std::string& fdId, int channelId,
  bool isSub, unsigned& monitorId, std::string& videoAddr, unsigned& videoPort,
      std::string& audioAddr, unsigned& audioPort);

  // 关闭实时码流
  void closeRealplayStream(unsigned monitorId);

  // 打开回放流地址
  unsigned openPlaybackStream(const std::string& fdId, int channelId,
  bool isCenter, unsigned beginTime, unsigned endTime, unsigned& monitorId,
      std::string& videoAddr, unsigned& videoPort, std::string& audioAddr,
      unsigned& audioPort);

  // 停止回放流
  void closePlaybackStream(unsigned monitorId, bool isCenter);

  // 控制回放
  unsigned controlPlayback(unsigned monitorId, bool isCenter, unsigned controlId,
      const std::string& action, const std::string& param);

  // 获取码流并设置码流回调
  unsigned startStream(const std::string& addr, unsigned port,
      fStreamCallBack streamCallback,
      fStreamConnectStatusCallBack streamConnectStatusCallback, void* pUser,
      unsigned& streamId);

  // 停止获取码流
  void stopStream(unsigned streamId);

  // 控制指令
  void sendCmd(const std::string& fdId, int channelId, unsigned controlType,
      unsigned param1, unsigned param2);

  // 开始接收实时报警
  void startReceiveRealAlarm(fRealAlarmCallBack realAlarmCallback) {
    _realAlarmCallback = realAlarmCallback;
  }

  // 停止接收实时报警
  void stopReceiveRealAlarm() {
    _realAlarmCallback = nullptr;
  }

  // 封装的操作接口结束------------------------------------------------

  // 启动uas
  bool startUasClient(const std::string& serverAddr, unsigned port);
  // 关闭uas
  void stopUasClient();
  // 中断uas
  void pauseUasClient();
  // 恢复uas
  bool resumeUasClient();

private:
  // uas连接状态回调
  void onUasConnectStatus(bool isConnected);
  // 实时报警回调
  void onRealAlarm(const std::string& fdId, int channelId, unsigned alarmType,
      unsigned param1, unsigned param2);

private:
  ThreadPool _pool;
  std::atomic<bool> _uasConnected;  // uas连接状态
  std::unique_ptr<UasClient> _uasClientPtr;  // uas客户端线程
  fUasConnectStatusCallBack _uasConnectStatusCallback;  // uas状态回调函数
  fRealAlarmCallBack _realAlarmCallback;  // 实时报警回调

  StreamSessionManager _streamSessionManager; // 流sessions
};

} /* namespace Dream */

#endif /* MAINMODULE_H_ */
