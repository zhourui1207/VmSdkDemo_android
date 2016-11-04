//
// Created by zhou rui on 16/11/2.
//
#include <Android/Log.h>
#include <stdio.h>

#include "Decoder.h"
#include "PayloadTypeDef.h"
#include "../sdl/SDL.h"

namespace Dream {

  Decoder::Decoder()
      : _codecId(AV_CODEC_ID_H264), _inited(false), _isRGB(false), _codecContext(nullptr),
        _codecParserContext(nullptr), _frame(nullptr), _width(0), _height(0), colortab(nullptr),
        u_b_tab(
            nullptr), u_g_tab(nullptr), v_g_tab(nullptr), v_r_tab(nullptr), rgb_2_pix(nullptr),
        r_2_pix(
            nullptr), g_2_pix(nullptr), b_2_pix(nullptr) {

  }

  Decoder::~Decoder() {
    Uninit();
  }

  bool Decoder::Init(unsigned payloadType, bool isRGB) {
    std::lock_guard<std::mutex> lock(_mutex);

    static bool isRegisterAll = false;
    if (!isRegisterAll) {
      __android_log_print(ANDROID_LOG_WARN, "Decoder", "开始注册所有");
      printf("Decoder 开始注册所有！\n");
      avcodec_register_all();
      isRegisterAll = true;
    }

    SDL_Init(0);

    if (_inited) {
      return true;
    }

    _isRGB = isRGB;
    if (_isRGB) {
      CreateYUVTab_16();
    }

    if (payloadType == MEDIA_PAYLOAD_H265) {
      _codecId = AV_CODEC_ID_HEVC;
    }

    // 查找解码器
    AVCodec *codec = avcodec_find_decoder(_codecId);
    if (!codec) {
      printf("解码器[%d] 未找到\n", _codecId);
      return false;
    }

    // 分配解码器上下文
    _codecContext = avcodec_alloc_context3(codec);
    if (!_codecContext) {
      printf("无法分配解码器上下文\n");
      return false;
    }

    // 初始化解析上下文
    _codecParserContext = av_parser_init(_codecId);
    if (!_codecParserContext) {
      printf("无法初始化解析上下文\n");
      return false;
    }

    if (codec->capabilities & CODEC_CAP_TRUNCATED) {
      _codecContext->flags |= CODEC_FLAG_TRUNCATED;
    }

    // 打开解码器
    if (avcodec_open2(_codecContext, codec, NULL) < 0)//
    {
      printf("打开解码器失败\n");
      return false;
    }

    // 分配帧
    _frame = av_frame_alloc();

    // 初始化包
    av_init_packet(&avPacket);

    _inited = true;
    return _inited;
  }

  void Decoder::Uninit() {
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_inited) {
      return;
    }

    if (nullptr != _frame) {
      av_frame_free(&_frame);
      _frame = nullptr;
    }

    if (nullptr != _codecParserContext) {
      av_parser_close(_codecParserContext);
      _codecParserContext = nullptr;
    }

    if (nullptr != _codecContext) {
      avcodec_close(_codecContext);
      av_free(_codecContext);
      _codecContext = nullptr;
    }

    if (_isRGB) {
      DeleteYUVTab();
    }

    _inited = false;
  }

  void pgm_save2(unsigned char *buf,int wrap, int xsize,int ysize,uint8_t *pDataOut)
  {
    int i;
    for(i=0;i<ysize;i++)
    {
      memcpy(pDataOut+i*xsize, buf + /*(ysize-i)*/i * wrap, xsize);
    }

  }

  bool Decoder::DecodeNalu(const char *inData, int inLen, char *outData, int &outLen) {
    if (!_inited || nullptr == _codecContext || nullptr == _codecParserContext) {
      return false;
    }

    if (inLen > 0) {
      avPacket.data = (uint8_t *) inData;
      avPacket.size = inLen;

      if (avPacket.size > 0) {
        int ret = avcodec_send_packet(_codecContext, &avPacket);

        if (ret != 0) {
          printf("输入包失败！\n");
          return false;
        }

        ret = avcodec_receive_frame(_codecContext, _frame);

        if (_codecContext->framerate.den != 0 && _codecContext->framerate.num != 0) {
          _framerate = _codecContext->framerate.num / _codecContext->framerate.den;
        } else {
          _framerate = 25;
        }

        if (ret == 0) {
          _width = _frame->width;
          _height = _frame->height;

          if (_isRGB) {
            DisplayYUV_16((unsigned int *)outData, _frame->data[0], _frame->data[1], _frame->data[2],
                          _codecContext->width, _codecContext->height, _frame->linesize[0],
                          _frame->linesize[1], _codecContext->width);
          } else {
            pgm_save2(_frame->data[0], _frame->linesize[0], _codecContext->width, _codecContext->height,
                      (uint8_t *) outData);
            pgm_save2(_frame->data[1], _frame->linesize[1], _codecContext->width/2, _codecContext->height/2,
                      (uint8_t *) outData + _codecContext->width*_codecContext->height);
            pgm_save2(_frame->data[2], _frame->linesize[2], _codecContext->width/2, _codecContext->height/2,
                      (uint8_t *) outData + _codecContext->width*_codecContext->height*5/4);
//            memcpy(outData, _frame->data[0], _frame->linesize[0]);
//            memcpy(outData + _frame->linesize[0], _frame->data[1], _frame->linesize[1]);
//            memcpy(outData + _frame->linesize[0] + _frame->linesize[1], _frame->data[2], _frame->linesize[2]);
          }

          outLen = _codecContext->width * _codecContext->height * 3 / 2;

          return true;
        }

      }
    }

    return false;
  }


  void Decoder::DeleteYUVTab() {
    av_free(colortab);
    av_free(rgb_2_pix);
  }

  void Decoder::CreateYUVTab_16() {
    int i;
    int u, v;

    colortab = (int *) av_malloc(4 * 256 * sizeof(int));
    u_b_tab = &(colortab[0 * 256]);
    u_g_tab = &(colortab[1 * 256]);
    v_g_tab = &(colortab[2 * 256]);
    v_r_tab = &(colortab[3 * 256]);

    for (i = 0; i < 256; i++) {
      u = v = (i - 128);

      u_b_tab[i] = (int) (1.772 * u);
      u_g_tab[i] = (int) (0.34414 * u);
      v_g_tab[i] = (int) (0.71414 * v);
      v_r_tab[i] = (int) (1.402 * v);
    }

    rgb_2_pix = (unsigned int *) av_malloc(3 * 768 * sizeof(unsigned int));

    r_2_pix = &(rgb_2_pix[0 * 768]);
    g_2_pix = &(rgb_2_pix[1 * 768]);
    b_2_pix = &(rgb_2_pix[2 * 768]);

    for (i = 0; i < 256; i++) {
      r_2_pix[i] = 0;
      g_2_pix[i] = 0;
      b_2_pix[i] = 0;
    }

    for (i = 0; i < 256; i++) {
      r_2_pix[i + 256] = (i & 0xF8) << 8;
      g_2_pix[i + 256] = (i & 0xFC) << 3;
      b_2_pix[i + 256] = (i) >> 3;
    }

    for (i = 0; i < 256; i++) {
      r_2_pix[i + 512] = 0xF8 << 8;
      g_2_pix[i + 512] = 0xFC << 3;
      b_2_pix[i + 512] = 0x1F;
    }

    r_2_pix += 256;
    g_2_pix += 256;
    b_2_pix += 256;
  }

  void Decoder::DisplayYUV_16(unsigned int *pdst1, unsigned char *y, unsigned char *u,
                              unsigned char *v, int width, int height, int src_ystride,
                              int src_uvstride, int dst_ystride) {
    int i, j;
    int r, g, b, rgb;

    int yy, ub, ug, vg, vr;

    unsigned char *yoff;
    unsigned char *uoff;
    unsigned char *voff;

    unsigned int *pdst = pdst1;

    int width2 = width / 2;
    int height2 = height / 2;

    if (width2 > width / 2) {
      width2 = _width / 2;

      y += (width - _width) / 4 * 2;
      u += (width - _width) / 4;
      v += (width - _width) / 4;
    }

    if (height2 > _height)
      height2 = _height;

    for (j = 0; j < height2; ++j) {
      yoff = y + j * 2 * src_ystride;
      uoff = u + j * src_uvstride;
      voff = v + j * src_uvstride;

      for (i = 0; i < width2; ++i) {
        yy = *(yoff + (i << 1));
        ub = u_b_tab[*(uoff + i)];
        ug = u_g_tab[*(uoff + i)];
        vg = v_g_tab[*(voff + i)];
        vr = v_r_tab[*(voff + i)];

        b = yy + ub;
        g = yy - ug - vg;
        r = yy + vr;

        rgb = r_2_pix[r] + g_2_pix[g] + b_2_pix[b];

        yy = *(yoff + (i << 1) + 1);
        b = yy + ub;
        g = yy - ug - vg;
        r = yy + vr;

        pdst[(j * dst_ystride + i)] =
            (rgb) + ((r_2_pix[r] + g_2_pix[g] + b_2_pix[b]) << 16);

        yy = *(yoff + (i << 1) + src_ystride);
        b = yy + ub;
        g = yy - ug - vg;
        r = yy + vr;

        rgb = r_2_pix[r] + g_2_pix[g] + b_2_pix[b];

        yy = *(yoff + (i << 1) + src_ystride + 1);
        b = yy + ub;
        g = yy - ug - vg;
        r = yy + vr;

        pdst[((2 * j + 1) * dst_ystride + i * 2) >> 1] =
            (rgb) + ((r_2_pix[r] + g_2_pix[g] + b_2_pix[b]) << 16);
      }
    }
  }

}