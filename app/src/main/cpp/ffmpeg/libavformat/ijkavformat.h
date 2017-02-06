/*
 * ijkavformat.h
 *
 * Copyright (c) 2003 Fabrice Bellard
 * Copyright (c) 2013 Zhang Rui <bbcallen@gmail.com>
 *
 * This file is part of ijkPlayer.
 *
 * ijkPlayer is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * ijkPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with ijkPlayer; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef AVFORMAT_IJKAVFORMAT_H
#define AVFORMAT_IJKAVFORMAT_H

#include "url.h"

#define AV_PKT_FLAG_DISCONTINUITY 0x0100

/**
 * Injection
 */

typedef struct IJKAVInject_OnUrlOpenData {
    size_t size;
    char url[4096];
    /* in, out */
    int segment_index;
    /* in, default = 0 */
    int retry_counter;
    /* in */

    int is_handled;
    /* out, default = false */
    int is_url_changed; /* out, default = false */
} IJKAVInject_OnUrlOpenData;

/**
 * Resolve segment url from concatdec
 *
 * @param data      IJKAVInject_OnUrlOpenData*
 * @param data_size size of IJKAVInject_OnUrlOpenData;
 * @return 0 if OK, AVERROR_xxx otherwise
 */
#define IJKAVINJECT_CONCAT_RESOLVE_SEGMENT  0x10000

/**
 * Protocol open event
 *
 * @param data      IJKAVInject_OnUrlOpenData*
 * @param data_size size of IJKAVInject_OnUrlOpenData;
 * @return 0 if OK, AVERROR_xxx otherwise
 */
#define IJKAVINJECT_ON_TCP_OPEN         0x10001
#define IJKAVINJECT_ON_HTTP_OPEN        0x10002
#define IJKAVINJECT_ON_HTTP_RETRY       0x10003
#define IJKAVINJECT_ON_LIVE_RETRY       0x10004


/**
 * Statistic
 */
typedef struct IJKAVInject_AsyncStatistic {
    size_t size;
    int64_t buf_backwards;
    int64_t buf_forwards;
    int64_t buf_capacity;
} IJKAVInject_AsyncStatistic;

#define IJKAVINJECT_ASYNC_STATISTIC     0x11000

typedef struct IJKAVInject_AsyncReadSpeed {
    size_t size;
    int is_full_speed;
    int64_t io_bytes;
    int64_t elapsed_milli;
} IJKAVInject_AsyncReadSpeed;

#define IJKAVINJECT_ASYNC_READ_SPEED    0x11001

typedef struct IJKAVInject_IpAddress {
    int error;
    int family;
    char ip[96];
    int port;
} IJKAVInject_IpAddress;
#define IJKAVINJECT_DID_TCP_CONNECT     0x12002

// AVAppHttpEvent
#define IJKAVINJECT_WILL_HTTP_OPEN      0x12100
#define IJKAVINJECT_DID_HTTP_OPEN       0x12101
#define IJKAVINJECT_WILL_HTTP_SEEK      0x12102
#define IJKAVINJECT_DID_HTTP_SEEK       0x12103

// AVAppIOTraffic
#define IJKAVINJECT_ON_IO_TRAFFIC       0x12204

typedef int (*IjkAVInjectCallback)(void *opaque, int message, void *data, size_t data_size);

IjkAVInjectCallback ijkav_register_inject_callback(IjkAVInjectCallback callback);

IjkAVInjectCallback ijkav_get_inject_callback(void);

int ijkav_register_ijkmediadatasource_protocol(URLProtocol *protocol, int protocol_size);

#endif
