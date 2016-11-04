/*
 * VmType.h
 *
 *  Created on: 2016年10月18日
 *      Author: zhourui
 */

#ifndef VMTYPE_H_
#define VMTYPE_H_

#define CALLBACK

// 实时报警回调  报警类型
#define ALARM_TYPE_EXTERNEL 1  // 外部告警
#define ALARM_TYPE_DETECTION 2  // 移动侦测
#define ALARM_TYPE_LOST 3  // 视频丢失
#define ALARM_TYPE_MASKING 4  // 视频遮盖

// 码流回调  码流数据类型
#define STREAM_TYPE_FIX 0  // 混合流
#define STREAM_TYPE_VIDEO 1  // 视频流
#define STREAM_TYPE_AUDIO 2  // 音频流

// 录像回放控制
#define CONTROLID_DEFAULT 0  // 默认控制id
#define ACTION_PLAY "PLAY"  // 播放／继续  param:""
#define ACTION_PAUSE "PAUSE"  // 暂停 param:""
#define ACTION_POS "POS"  // 定位到某个时间 param:起止时间内的时间点，单位秒
#define ACTION_SPEED "SPEED"  // 播放速度 param:播放倍率，支持 0.125, 0.25, 0.5, 1, 2, 4, 8

// 回调函数定义开始---------------------------------------------------------
// 实时报警回调函数
// pFdId：设备id； nChannel：通道号； uAlarmType：报警类型； uParam1：参数1； uParam2：参数2
typedef void (CALLBACK *fRealAlarmCallBack)(const char* pFdId, int nChannelId,
    unsigned uAlarmType, unsigned uParam1, unsigned uParam2);
// uas连接状态回调函数
// bIsConnected：true 已连接  false 断开
typedef void (CALLBACK *fUasConnectStatusCallBack)(bool bIsConnected);
// 码流回调函数
// uStreamId：码流id； uStreamType：数据类型； payloadType：码流承载类型；pBuffer：数据指针；
// nLen：数据长度； uTimeStamp：时间戳； pUser：用户参数；sequenceNumber：序列号；isMark：是否是结束
typedef void (CALLBACK *fStreamCallBack)(unsigned uStreamId,
    unsigned uStreamType, char payloadType, char* pBuffer, int nLen,
    unsigned uTimeStamp, unsigned short sequenceNumber, bool isMark, void* pUser);
// stream码流连接状态回调
// uStreamId：码流id； bIsConnected：true 已连接 false 断开； pUser：用户参数
typedef void (CALLBACK *fStreamConnectStatusCallBack)(unsigned uStreamId,
    bool bIsConnected, void* pUser);
// 回调函数定义结束---------------------------------------------------------

// 数据结构定义开始---------------------------------------------------------
// 行政树
typedef struct VmDepTree {
  int nDepId;
  char sDepName[64];
  int nParentId;
  unsigned uOnlineChannelCounts;
  unsigned uOfflineChannelCounts;
} TVmDepTree;

// 视频通道
typedef struct VmChannel {
  int nDepId;
  char sFdId[32];
  int nChannelId;
  unsigned uChannelType;
  char sChannelName[64];
  unsigned uIsOnLine;
  unsigned uVideoState;
  unsigned uChannelState;
  unsigned uRecordState;
} TVmChannel;

// 录像文件
typedef struct VmRecord {
  unsigned uBeginTime;
  unsigned uEndTime;
  char sPlaybackUrl[256];
  char sDownloadUrl[256];
} TVmRecord;

// 历史报警
typedef struct VmAlarm {
  char sAlarmId[40];
  char sFdId[32];
  char sFdName[64];
  int nChannelId;
  char sChannelName[64];
  unsigned uChannelBigType;
  char sAlarmTime[40];
  unsigned uAlarmCode;
  char sAlarmName[64];
  char sAlarmSubName[64];
  unsigned uAlarmType;
  char sPhotoUrl[256];
} TVmAlarm;
// 数据结构定义结束---------------------------------------------------------

#endif /* VMTYPE_H_ */
