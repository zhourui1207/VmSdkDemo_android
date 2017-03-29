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
        int encodePCM(const char *inData, int inLen);

    private:
        int _samplerate;
        int _frameCount;

        AVCodecID _codecId;
        bool _inited;
        AVCodecContext *_codecContext;
        AVFrame *_frame;
        std::mutex _mutex;
        AVPacket avPacket;

    };
}

#endif

#endif //VMSDKDEMO_ANDROID_AACENCODER_H
