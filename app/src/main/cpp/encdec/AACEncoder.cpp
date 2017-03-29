//
// Created by zhou rui on 17/3/26.
//
#ifdef PHONE_PLATEFORM  // 如果是手机操作平台

#include <stdio.h>

#include "AACEncoder.h"
#include "../util/public/platform.h"

namespace Dream {

    AACEncoder::AACEncoder()
    : _samplerate(8000), _frameCount(0), _codecId(AV_CODEC_ID_AAC), _inited(false), _codecContext(nullptr), _frame(nullptr) {

    }

    AACEncoder::~AACEncoder() {
        Uninit();
    }

    bool AACEncoder::Init(int samplerate) {
        std::lock_guard<std::mutex> lock(_mutex);

        static bool isRegisterAll = false;
        if (!isRegisterAll) {
            LOGW(TAG, "开始注册所有\n");
            avcodec_register_all();
            // 初始化sdl
            isRegisterAll = true;
        }

        if (_inited) {
            return true;
        }

        _samplerate = samplerate;

        // 查找解码器
        AVCodec *codec = avcodec_find_encoder(_codecId);
        if (!codec) {
            LOGW(TAG, "编码器[%d] 未找到\n", _codecId);
            return false;
        }

        // 分配解码器上下文
        _codecContext = avcodec_alloc_context3(codec);
        if (!_codecContext) {
            LOGW(TAG, "无法分配解码器上下文\n");
            return false;
        }

        /* put sample parameters */
        _codecContext->sample_rate = _samplerate;
        _codecContext->channels = 1;
        _codecContext->sample_fmt = AV_SAMPLE_FMT_S16;
        /* select other audio parameters supported by the encoder */
        _codecContext->channel_layout = AV_CH_LAYOUT_STEREO;

        //准备好了编码器和编码器上下文环境，现在可以打开编码器了
        //根据编码器上下文打开编码器
        // 打开解码器
        if (avcodec_open2(_codecContext, codec, NULL) < 0)//
        {
            LOGW(TAG, "打开编码器失败\n");
            return false;
        }

        // 分配帧
        _frame = av_frame_alloc();
        _frame->format = AV_SAMPLE_FMT_S16;
        _frame->nb_samples = 1024;
        _frame->sample_rate = _samplerate;
        _frame->channels = 1;

        // 初始化包
        av_init_packet(&avPacket);

        _inited = true;
        return _inited;
    }

    void AACEncoder::Uninit() {
        std::lock_guard<std::mutex> lock(_mutex);
        if (!_inited) {
            return;
        }

        if (nullptr != _frame) {
            av_frame_free(&_frame);
            _frame = nullptr;
        }

        if (nullptr != _codecContext) {
            avcodec_close(_codecContext);
            av_free(_codecContext);
            _codecContext = nullptr;
        }

        _inited = false;
    }

    bool AACEncoder::EncodePCM2AAC(const char *inData, int inLen, char *outData, int &outLen) {
        if (encodePCM(inData, inLen) == 0) {
            if (outData && outLen >= avPacket.size) {
                memcpy(outData, avPacket.data, (size_t) avPacket.size);
                return true;
            }
        }
        return false;
    }

    int AACEncoder::encodePCM(const char *inData, int inLen) {
        int ret = -1;
        if (inLen > 0) {
//            avPacket.data = (uint8_t *) inData;
//            avPacket.size = inLen;
            _frame->data[0] = (uint8_t *) inData;
            _frame->linesize[0] = inLen;

            // 初始化包
            av_init_packet(&avPacket);

            _frame->pts = _frameCount;

            ret = avcodec_send_frame(_codecContext, _frame);

            if (ret != 0) {
                LOGW(TAG, "编码失败\n");
                return -1;
            }

            ret = avcodec_receive_packet(_codecContext, &avPacket);

        }
        return ret;
    }
}

#endif
