/*
 * ControlType.h
 *
 *  Created on: 2016年10月11日
 *      Author: zhourui
 */

#ifndef ALARMTYPE_H_
#define ALARMTYPE_H_

namespace Dream {

enum ALARM_TYPE {
  ALARM_TYPE_EXTERNEL = 0x01,  // 外部告警
  ALARM_TYPE_DETECTION = 0x02,  // 移动侦测
  ALARM_TYPE_LOST = 0x03,  // 视频丢失
  ALARM_TYPE_MASKING = 0x04  // 视频遮盖
};

} /* namespace Dream */

#endif /* CONTROLTYPE_H_ */
