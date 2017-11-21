/*
 * StreamData.h
 *
 *  Created on: 2016年9月29日
 *      Author: zhourui
 *
 *      码流数据
 */

#ifndef STREAMTDATA_H_
#define STREAMTDATA_H_

#include <cstring>
#include <stdio.h>
#include "public/platform.h"

namespace Dream {

// todo 看情况需要改成使用内存池提高性能
    class StreamData {
    public:
        enum STREAM_TYPE {
            STREAM_TYPE_FIX = 0,  // 混合流
            STREAM_TYPE_VIDEO = 1,  // 视频流
            STREAM_TYPE_AUDIO = 2  // 音频流
        };

    private:
        static const std::size_t STREAM_DATA_DEFAULT_SIZE = 100 * 1024;

    public:
        StreamData() = delete;

        StreamData(STREAM_TYPE streamType, std::size_t length, const char *data = nullptr)
                : _streamType(streamType),
                  _length(length > STREAM_DATA_DEFAULT_SIZE ? STREAM_DATA_DEFAULT_SIZE : length)
//                 , _data(nullptr)
        {
            if (length > STREAM_DATA_DEFAULT_SIZE) {  // 超出最大值
                LOGE("StreamData", "传入长度[%zd]超出允许的最大值[%zd]\n", length, STREAM_DATA_DEFAULT_SIZE);
            }
            if (data != nullptr) {
//                _data = new char[_length];
                memcpy(_data, data, _length);
            }
        }

        virtual ~StreamData() {
//            if (_data != nullptr) {
//                delete[] _data;
//                _data = nullptr;
//            }
        }

        STREAM_TYPE streamType() {
            return _streamType;
        }

        char *data() {
            return _data;
        }

        std::size_t length() {
            return _length;
        }

    private:
        STREAM_TYPE _streamType;
        std::size_t _length;
        char _data[STREAM_DATA_DEFAULT_SIZE];
//        char *_data;
    };

} /* namespace Dream */

#endif /* PACKETDATA_H_ */
