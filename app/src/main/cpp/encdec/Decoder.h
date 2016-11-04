//
// Created by zhou rui on 16/11/2.
//

#ifndef DREAM_DECODER_H
#define DREAM_DECODER_H

#include <mutex>

#ifdef __cplusplus
extern "C" {
#endif

#include "../ffmpeg/libavcodec/avcodec.h"

#ifdef __cplusplus
}
#endif

/**
 * 基于ffmpeg的解码器，播放264或265视频
 */
namespace Dream {

  class Decoder {
  public:
    Decoder();

    ~Decoder();

    bool Init(unsigned payloadType, bool isRGB);

    void Uninit();

    bool DecodeNalu(const char *inData, int inLen, char *outData, int &outLen);

    AVCodecID CodecId() {
      return _codecId;
    }

    int Width() {
      return _width;
    }

    int Height() {
      return _height;
    }

  private:
    void DeleteYUVTab();

    void CreateYUVTab_16();

    void DisplayYUV_16(unsigned int *pdst1, unsigned char *y, unsigned char *u,
                       unsigned char *v, int width, int height, int src_ystride, int src_uvstride,
                       int dst_ystride);

  private:
    AVCodecID _codecId;
    bool _inited;
    bool _isRGB;
    AVCodecContext *_codecContext;
    AVCodecParserContext *_codecParserContext;
    AVFrame *_frame;
    std::mutex _mutex;
    AVPacket avPacket;
    int _width;
    int _height;
    int _framerate;

    int *colortab;
    int *u_b_tab;
    int *u_g_tab;
    int *v_g_tab;
    int *v_r_tab;

    unsigned int *rgb_2_pix;
    unsigned int *r_2_pix;
    unsigned int *g_2_pix;
    unsigned int *b_2_pix;
  };

}
#endif //VMSDKDEMO_ANDROID_DECODER_H
