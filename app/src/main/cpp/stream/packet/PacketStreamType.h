/*
 * PacketMsgType.h
 *
 *  Created on: 2016年9月28日
 *      Author: zhourui
 */

#ifndef DREAM_PACKETSTREAMTYPE_H_
#define DREAM_PACKETSTREAMTYPE_H_

// 视频流
const unsigned MSG_STREAM_VIDEO = 0xd1e2f300;

// 音频流
const unsigned MSG_STREAM_AUDIO = 0xd1e2f301;

// 保活
const unsigned MSG_STREAM_ACTIVE = 0xd1e2f303;

// 鉴权包
const unsigned MSG_AUTH_REQ = 0xd1e2f312;

// 鉴权响应包
const unsigned MSG_AUTH_RESP = 0xd1e2f313;

#endif /* PACKETMSGTYPE_H_ */
