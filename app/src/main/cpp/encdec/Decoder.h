//
// Created by zhou rui on 16/11/2.
//

#ifndef DREAM_DECODER_H
#define DREAM_DECODER_H

#include <mutex>

#include "GLES2/gl2.h"
#include "EGL/egl.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "../ffmpeg/libavcodec/avcodec.h"

#ifdef __cplusplus
}
#endif

/**
 * 基于ffmpeg的软件解码器，播放264或265视频。 集成了opengles2.0的渲染功能
 * 提供两种帧解码方式
 * 1:解码成rgb格式，但使用较方便，上层能够直接使用rgb数据绘图
 *   在arm架构下效率较低，转换时需要大量cpu资源不推荐使用。
 * 2:解码成yuv格式，上层拿到数据可以自行转换，也可调用解码器的yuv绘图功能直接绘图，但需要初始化渲染层
 */

namespace Dream {

  class Decoder {
  public:
    Decoder();

    ~Decoder();


    bool Init(unsigned payloadType,  bool isRGB, void *nativeWindowType);

    void Uninit();

    bool DecodeNalu(const char *inData, int inLen, char *outData,
                        int &outLen);

    bool DecodeNalu2YUV(const char *inData, int inLen, int &width, int &height, char *outYData,
                        int &outYLen, char *outUData, int &outULen, char *outVData, int &outVLen);

    bool RenderInit(EGLNativeWindowType surface);

    bool RenderUninit();

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

    GLuint CreateProgram();

    GLuint CreateShader(int shaderType, const char *source);

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

    GLuint _program;
    GLint _position;
    GLint _coord;
    GLint _y;
    GLint _u;
    GLint _v;
    GLuint _rb;
    GLuint _colorBuffer;

    GLuint _texYId;
    GLuint _texUId;
    GLuint _texVId;

    GLuint _fbo;

    EGLDisplay _display;
    EGLSurface _surface;

    EGLNativeWindowType _nativeWindowType;
  };

}
#endif //VMSDKDEMO_ANDROID_DECODER_H
