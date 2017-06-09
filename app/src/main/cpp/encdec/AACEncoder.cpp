//
// Created by zhou rui on 17/3/26.
//
#ifdef PHONE_PLATEFORM  // 如果是手机操作平台

#include <stdio.h>

#include "AACEncoder.h"
#include "../util/public/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "libavformat/avformat.h"

#ifdef __cplusplus
}
#endif

namespace Dream {

    AACEncoder::AACEncoder()
            : _samplerate(8000), _frameCount(0), _codecId(AV_CODEC_ID_AAC), _inited(false),
              _codecContext(nullptr), _inputCodecContext(nullptr), _frame(nullptr), _convertFrame(
                    nullptr), _swrContext(nullptr), _frameBuf(nullptr), _frameCurrentSize(0),
              _frameTotalSize(0), _convertData(nullptr), _convertSize(0) {

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
            LOGE(TAG, "编码器[%d] 未找到\n", _codecId);
            return false;
        }

        // 分配解码器上下文
        _codecContext = avcodec_alloc_context3(codec);
        if (!_codecContext) {
            LOGE(TAG, "无法分配解码器上下文\n");
            return false;
        }

        /* put sample parameters */
        _codecContext->codec_type = AVMEDIA_TYPE_AUDIO;
        _codecContext->sample_fmt = AV_SAMPLE_FMT_FLTP;  // 采样格式32bit
        _codecContext->sample_rate = _samplerate;
        _codecContext->channels = 1;
        _codecContext->channel_layout = AV_CH_LAYOUT_MONO;  // 单声道
        _codecContext->profile = FF_PROFILE_AAC_LOW;  // aac-lc
        _codecContext->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

        //准备好了编码器和编码器上下文环境，现在可以打开编码器了
        //根据编码器上下文打开编码器
        // 打开解码器
        if (avcodec_open2(_codecContext, codec, NULL) < 0)//
        {
            LOGE(TAG, "打开编码器失败\n");
            return false;
        }

        // 分配帧
        _convertFrame = av_frame_alloc();
        _convertFrame->format = _codecContext->sample_fmt;
        _convertFrame->nb_samples = _codecContext->frame_size;  // 默认1024
        _convertFrame->sample_rate = _samplerate;
        _convertFrame->channels = _codecContext->channels;

        _frame = av_frame_alloc();
        _frame->format = AV_SAMPLE_FMT_S16;
        _frame->nb_samples = _convertFrame->nb_samples;
        _frame->sample_rate = _convertFrame->sample_rate;
        _frame->channels = _convertFrame->channels;

        // 初始化包
        av_init_packet(&_avPacket);
        _avPacket.data = nullptr;
        _avPacket.size = 0;

        // 初始化音频转换器
        _swrContext = swr_alloc_set_opts(NULL,  // we're allocating a new context
                                         AV_CH_LAYOUT_MONO,  // out_ch_layout
                                         AV_SAMPLE_FMT_FLTP,    // out_sample_fmt
                                         _samplerate,                // out_sample_rate
                                         AV_CH_LAYOUT_MONO, // in_ch_layout
                                         AV_SAMPLE_FMT_S16,   // in_sample_fmt
                                         _samplerate,                // in_sample_rate
                                         0,                    // log_offset
                                         NULL);                // log_ctx

        if (!_swrContext || swr_init(_swrContext) < 0) {
            LOGE(TAG, "创建音频转换器失败!\n");
            return false;
        }

        /* 存储原始数据 */
        _frameTotalSize = (size_t) av_samples_get_buffer_size(nullptr, _codecContext->channels,
                                                              _codecContext->frame_size,
                                                              AV_SAMPLE_FMT_S16, 0);
        _frameBuf = (uint8_t *) av_malloc(_frameTotalSize);

        LOGW(TAG, "frameTotalSize=[%d]\n", _frameTotalSize);

        if (avcodec_fill_audio_frame(_frame, _codecContext->channels, AV_SAMPLE_FMT_S16,
                                     _frameBuf, _frameTotalSize, 0) < 0) {
            LOGE(TAG, "分配原样本空间失败!\n");
            return false;
        }

        /* 分配空间,存储转码后结果*/
        _convertData = (uint8_t **) calloc((size_t) _codecContext->channels, sizeof(*_convertData));
        _convertSize = (size_t) av_samples_alloc(_convertData, nullptr, _codecContext->channels,
                                                 _codecContext->frame_size, AV_SAMPLE_FMT_FLTP, 0);

        LOGW(TAG, "convertSize=[%d]\n", _convertSize);

        if (avcodec_fill_audio_frame(_convertFrame, _codecContext->channels, AV_SAMPLE_FMT_FLTP,
                                     *_convertData, _convertSize, 0) < 0) {
            LOGE(TAG, "分配转换后样本空间失败!\n");
            return false;
        }

        _inited = true;
        return _inited;
    }

    void AACEncoder::Uninit() {
        std::lock_guard<std::mutex> lock(_mutex);
        if (!_inited) {
            return;
        }

        if (nullptr != _swrContext) {
            swr_free(&_swrContext);
            _swrContext = nullptr;
        }

        if (nullptr != _frame) {
            av_frame_free(&_frame);
            _frame = nullptr;
        }

        if (nullptr != _convertFrame) {
            av_frame_free(&_convertFrame);
            _convertFrame = nullptr;
        }

        if (nullptr != _codecContext) {
            avcodec_close(_codecContext);
            av_free(_codecContext);
            _codecContext = nullptr;
        }

        if (nullptr != _convertData) {
            free(_convertData);
            _convertData = nullptr;
        }

        if (nullptr != _frameBuf) {
            av_free(_frameBuf);
            _frameBuf = nullptr;
        }

        _inited = false;
    }

    bool AACEncoder::EncodePCM2AAC(const char *inData, int inLen, char *outData, int &outLen) {
        if (nullptr == inData || nullptr == _frameBuf) {
            return false;
        }

//        LOGW(TAG, "inLen=[%d], _frameCurrentSize=[%d]\n", inLen, _frameCurrentSize);
        if (inLen + _frameCurrentSize < _frameTotalSize) {  // 拼帧
            memcpy(_frameBuf + _frameCurrentSize, inData, (size_t) inLen);
            _frameCurrentSize += inLen;
            return false;
        }

        memcpy(_frameBuf + _frameCurrentSize, inData, _frameTotalSize - _frameCurrentSize);
        size_t leaveLen = _frameCurrentSize + inLen - _frameTotalSize;  // 剩余长度

        int ret = encodePCM();

        _frameCurrentSize = 0;
        if ((leaveLen > 0) && (leaveLen < _frameTotalSize)) {  // 还有剩余长度
            memcpy(_frameBuf, inData + inLen - leaveLen, leaveLen);
            _frameCurrentSize += leaveLen;
        } else if (leaveLen >= _frameTotalSize) {
            LOGE(TAG, "pcm size is too long:%d!\n", inLen);
        }

        if (ret == 0 && _avPacket.data && outData && outLen >= _avPacket.size) {
            memcpy(outData, _avPacket.data, (size_t) _avPacket.size);
            outLen = _avPacket.size;
//            LOGE(TAG, "extradata_size=[%d]\n", _codecContext->extradata_size);
//            for (int i = 0; i < _codecContext->extradata_size; ++i) {
//                LOGE(TAG, "extradata[%d]=[%d]\n", i, _codecContext->extradata);
//            }
            return true;
        }

        return false;
    }

    int AACEncoder::encodePCM() {
        int ret = -1;

        if (_swrContext) {
//            LOGW(TAG, "convert begin\n");

            int outSampleNum = swr_convert(_swrContext, _convertFrame->data,
                                           _codecContext->frame_size,
                                           (const uint8_t **) _frame->data,
                                           _codecContext->frame_size);

//            LOGW(TAG, "convert success, outSampleNum=[%d]\n", outSampleNum);

            if (outSampleNum < 0) {
                LOGE(TAG, "convert failed!\n");
                return ret;
            }

            int dstBufsize = av_samples_get_buffer_size(_convertFrame->linesize,
                                                        _codecContext->channels,
                                                        outSampleNum, _codecContext->sample_fmt, 0);

            if (dstBufsize < 0) {
                LOGE(TAG, "av_samples_get_buffer_size failed dstBufsize=[%d]!\n", dstBufsize);
                return ret;
            }
//
//            LOGW(TAG, "dstBufsize=[%d], linesize=[%d]\n", dstBufsize, _convertFrame->linesize[0]);

//                _frame->data[0] = dstData[0];
//                _frame->linesize[0] = ret;

//            _convertFrame->nb_samples = outSampleNum;

            ret = avcodec_send_frame(_codecContext, _convertFrame);

//            LOGW(TAG, "avcodec_send_frame ret=[%d]\n", ret);

            if (ret != 0) {
                LOGE(TAG, "编码失败\n");
                return -1;
            }

            // 初始化包
            av_init_packet(&_avPacket);

            _avPacket.data = nullptr;
            _avPacket.size = 0;

//            LOGW(TAG, "av_init_packet success!\n");

            ret = avcodec_receive_packet(_codecContext, &_avPacket);
//            LOGW(TAG, "avcodec_receive_packet ret=[%d]\n", ret);
        }
        return ret;
    }
}

#endif
