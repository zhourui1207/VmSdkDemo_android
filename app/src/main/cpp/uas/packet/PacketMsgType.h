/*
 * PacketMsgType.h
 *
 *  Created on: 2016年9月28日
 *      Author: zhourui
 */

#ifndef PACKETMSGTYPE_H_
#define PACKETMSGTYPE_H_

// 错误码
const unsigned STATUS_OK = 0x00;  // 成功

const unsigned STATUS_LOGIN_AUTH = 0x01;  // 登录鉴权错误
const unsigned STATUS_LOGIN_MULTIMES = 0x02;  //  重复登录错误

const unsigned STATUS_TOO_SESSION = 0x03;  // 监控或对讲session过多
const unsigned STATUS_NOT_EXIST_SESSION = 0x04;  // session不存在

// 登录错误
const unsigned STATUS_ACCESS_DBMS_FAILED = 0x0226;  // 访问dbms错误
const unsigned UNKOWN_ERR = 0x0227;  // 未知错误
const unsigned USER_NO_EXIST = 0x0228;  // 用不存在
const unsigned NEED_USB = 0x0229;  // 需要usb
const unsigned NEED_SMS = 0x022a;  // 需要短信
const unsigned SMS_ERR = 0x022b;  // 短信错误
const unsigned ACCOUNT_OR_PWD_ERR = 0x022c;  // 用户名或密码错误
const unsigned DB_ERR = 0x022d;  // 数据库错误

const unsigned STATUS_GEN_ERR = 0xff;  // 一般错误

const unsigned STATUS_TIMEOUT_ERR = 0xf004;  // 超时
const unsigned STATUS_SEND_FAILED = 0xf005;  // 发送失败

// 登录
const unsigned MSG_LOGIN_REQ = 0x0201;
const unsigned MSG_LOGIN_RESP = 0x0202;

// 登出
const unsigned MSG_LOGOUT_REQ = 0x0203;
const unsigned MSG_LOGOUT_RESP = 0x0204;

// 心跳
const unsigned MSG_HEARTBEAT_REQ = 0x0205;
const unsigned MSG_HEARTBEAT_RESP = 0x0206;

// 获取行政树
const unsigned MSG_GET_DEP_TREE_REQ = 0x0211;
const unsigned MSG_GET_DEP_TREE_RESP = 0x0212;

// 获取通道
const unsigned MSG_GET_DEP_CHANNEL_REQ = 0x0213;
const unsigned MSG_GET_DEP_CHANNEL_RESP = 0x0214;

// 获取录像
const unsigned MSG_GET_RECORD_REQ = 0x0215;
const unsigned MSG_GET_RECORD_RESP = 0x0216;

// 预览
const unsigned MSG_START_MONITOR_REQ = 0x0231;
const unsigned MSG_START_MONITOR_RESP = 0x0232;
const unsigned MSG_STOP_MONITOR_REQ = 0x0233;
const unsigned MSG_STOP_MONITOR_RESP = 0x0234;

// 对讲
const unsigned MSG_START_TALK_REQ = 0x0235;
const unsigned MSG_START_TALK_RESP = 0x0236;
const unsigned MSG_STOP_TALK_REQ = 0x0237;
const unsigned MSG_STOP_TALK_RESP = 0x0238;

// 回放
const unsigned MSG_START_PLAYBACK_REQ = 0X0239;
const unsigned MSG_START_PLAYBACK_RESP = 0x023A;
const unsigned MSG_STOP_PLAYBACK_REQ = 0X023B;
const unsigned MSG_STOP_PLAYBACK_RESP = 0x023C;
const unsigned MSG_CONTROL_PLAYBACK_REQ = 0X023D;
const unsigned MSG_CONTROL_PLAYBACK_RESP = 0x023E;

// 控制
const unsigned MSG_CONTROL_REQ = 0x0251;
const unsigned MSG_CONTROL_RESP = 0x0252;

// 报警推送消息
const unsigned MSG_ALARM_NOTIFY= 0x0261;

// 获取报警
const unsigned MSG_GET_ALARM_REQ = 0x0271;
const unsigned MSG_GET_ALARM_RESP = 0x0272;

#endif /* PACKETMSGTYPE_H_ */
