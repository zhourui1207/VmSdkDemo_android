/*
 * CrossPlatform.h
 *
 *  Created on: 2016年9月12日
 *      Author: zhourui
 */

#ifndef PLATFORM_H_
#define PLATFORM_H_

#include <stdint.h>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <winsock.h>
#else
#include <arpa/inet.h>
#include <unistd.h>
#endif

/** sleep 跨平台 */
#if _WIN32
  inline void safeSleep(uint32_t ms) {
    ::Sleep(ms);
  }
#else
  inline void safeSleep(uint32_t ms) {
    usleep(ms*1000);
  }
#endif

#endif /* CROSSPLATFORM_H_ */
