//
// Created by zhou rui on 17/3/26.
//

#ifndef DREAM_AACENCODER_H
#define DREAM_AACENCODER_H

#ifdef PHONE_PLATEFORM  // 如果是手机操作平台

#include <mutex>

#ifdef __cplusplus
extern "C" {
#endif

#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"

#ifdef __cplusplus
}
#endif

namespace Dream {

    class AACEncoder {
    private:
        const char *TAG = "AACEncoder";
    public:
        AACEncoder();

        ~AACEncoder();

        bool Init(int samplerate);

        void Uninit();

        bool EncodePCM2AAC(const char *inData, int inLen, char *outData, int &outLen);

    private:
        int encodePCM();

    private:
        int _samplerate;
        int _frameCount;

        AVCodecID _codecId;
        bool _inited;
        AVCodecContext *_codecContext;
        AVCodecContext *_inputCodecContext;
        AVFrame *_frame;
        AVFrame *_convertFrame;
        std::mutex _mutex;
        AVPacket _avPacket;
        SwrContext *_swrContext;
        uint8_t *_frameBuf;  // 转换前音频
        size_t _frameCurrentSize;
        size_t _frameTotalSize;
        uint8_t **_convertData;
        size_t _convertSize;
    };
}

#endif

#endif //VMSDKDEMO_ANDROID_AACENCODER_H
