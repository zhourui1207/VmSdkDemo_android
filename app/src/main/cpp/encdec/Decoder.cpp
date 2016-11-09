//
// Created by zhou rui on 16/11/2.
//
#include <Android/Log.h>
#include <stdio.h>

#include "../util/public/platform.h"
#include "../util/PrintTimer.h"
#include "Decoder.h"
#include "PayloadTypeDef.h"

namespace Dream {

  static const char vertexShader[] = {
      "attribute vec4 aPosition;\n"
          " attribute vec2 aTexCoord;\n"
          " varying vec2 vTextureCoord;\n"
          " void main() {\n"
          " gl_Position = aPosition;\n"
          " vTextureCoord = aTexCoord;\n"
          "}\n"
  };

  static const char fragmentShader_yuv420p[] = {
      "precision mediump float;\n"
          "uniform sampler2D Ytex;\n"
          "uniform sampler2D Utex;\n"
          "uniform sampler2D Vtex;\n"
          "varying vec2 vTextureCoord;\n"
          "void main(void) {\n"
          " float nx,ny,r,g,b,y,u,v;\n"
          " mediump vec4 txl,ux,vx;"
          " nx=vTextureCoord[0];\n"
          " ny=vTextureCoord[1];\n"
          " y=texture2D(Ytex,vec2(nx,ny)).r;\n"
          " u=texture2D(Utex,vec2(nx,ny)).r;\n"
          " v=texture2D(Vtex,vec2(nx,ny)).r;\n"
          " y=1.1643*(y-0.0625);\n"
          " u=u-0.5;\n"
          " v=v-0.5;\n"
          " r=y+1.5958*v;\n"
          " g=y-0.39173*u-0.81290*v;\n"
          " b=y+2.017*u;\n"
          " gl_FragColor=vec4(r,g,b,1.0);\n"
          "}\n"
  };

  static float vertice_buffer[8] = {-1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f};
  static float fragment_buffer[8] = {0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f};

  Decoder::Decoder()
      : _codecId(AV_CODEC_ID_H264), _inited(false), _isRGB(false), _codecContext(nullptr),
        _codecParserContext(nullptr), _frame(nullptr), _width(0), _height(0), colortab(nullptr),
        u_b_tab(
            nullptr), u_g_tab(nullptr), v_g_tab(nullptr), v_r_tab(nullptr), rgb_2_pix(nullptr),
        r_2_pix(
            nullptr), g_2_pix(nullptr), b_2_pix(nullptr), _program(0), _position(0),
        _coord(0), _y(0), _u(0), _v(0), _fbo(0), _rb(0), _display(0), _surface(0), _colorBuffer(0),
        _nativeWindowType(0) {

  }

  Decoder::~Decoder() {
    Uninit();
  }

  bool Decoder::Init(unsigned payloadType, bool isRGB, void *nativeWindowType) {
    std::lock_guard<std::mutex> lock(_mutex);

    _nativeWindowType = (EGLNativeWindowType) nativeWindowType;
    static bool isRegisterAll = false;
    if (!isRegisterAll) {
//      int ret = SDL_VideoInit(NULL);
      __android_log_print(ANDROID_LOG_WARN, "Decoder", "开始注册所有");
      printf("Decoder 开始注册所有！\n");
      avcodec_register_all();
      // 初始化sdl
      isRegisterAll = true;
    }

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

    _display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (_display == EGL_NO_DISPLAY) {
      __android_log_print(ANDROID_LOG_ERROR, "GLES", "display failed");
    }


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

  void pgm_save2(unsigned char *buf, int wrap, int xsize, int ysize, uint8_t *pDataOut) {
    int i;
    for (i = 0; i < ysize; i++) {
      memcpy(pDataOut + i * xsize, buf + /*(ysize-i)*/i * wrap, xsize);
    }

  }

  bool Decoder::DecodeNalu(const char *inData, int inLen, char *outData, int &outLen) {
    if (!_inited || nullptr == _codecContext || nullptr == _codecParserContext) {
      return false;
    }

    PrintTimer timer("DecodeNalu");
    if (inLen > 0) {
      avPacket.data = (uint8_t *) inData;
      avPacket.size = inLen;

      __android_log_print(ANDROID_LOG_ERROR, "avcodec_receive_frame",
                          "bit_rate=%lld, width=%d, height=%d\n",
                          _codecContext->bit_rate, _codecContext->width, _codecContext->height);

      __android_log_print(ANDROID_LOG_ERROR, "avcodec_receive_frame",
                          "bit_rate=%lld, width=%d, height=%d, framenum=%d, den=%d",
                          _codecContext->bit_rate, _codecContext->width, _codecContext->height,
                          _codecContext->framerate.num, _codecContext->framerate.den);

      __android_log_print(ANDROID_LOG_ERROR, "avcodec_receive_frame",
                          "bit_rate=%lld",
                          _codecContext->bit_rate);
      __android_log_print(ANDROID_LOG_ERROR, "avcodec_receive_frame",
                          "framenum=%d, den=%d",
                          _codecContext->framerate.num, _codecContext->framerate.den);

      __android_log_print(ANDROID_LOG_ERROR, "avcodec_receive_frame", "width=%d, height=%d", _codecContext->width, _codecContext->height);

      if (avPacket.size > 0) {
        int ret = 0;
        {
          PrintTimer timer1("avcodec_send_packet");
          ret = avcodec_send_packet(_codecContext, &avPacket);
        }

        if (ret != 0) {
          printf("输入包失败！\n");
          return false;
        }

        {
          PrintTimer timer2("avcodec_receive_frame");
          ret = avcodec_receive_frame(_codecContext, _frame);
        }

        __android_log_print(ANDROID_LOG_ERROR, "avcodec_receive_frame",
                            "bit_rate=%d, width=%d, height=%d, framenum=%d, den=%d",
                            _codecContext->bit_rate, _codecContext->width, _codecContext->height,
                            _codecContext->framerate.num, _codecContext->framerate.den);

        __android_log_print(ANDROID_LOG_ERROR, "avcodec_receive_frame",
                            "bit_rate=%d",
                            _codecContext->bit_rate);
        __android_log_print(ANDROID_LOG_ERROR, "avcodec_receive_frame",
                            "framenum=%d, den=%d",
                            _codecContext->framerate.num, _codecContext->framerate.den);

        __android_log_print(ANDROID_LOG_ERROR, "avcodec_receive_frame", "width=%d, height=%d", _codecContext->width, _codecContext->height);

        if (_codecContext->framerate.den != 0 && _codecContext->framerate.num != 0) {
          _framerate = _codecContext->framerate.num / _codecContext->framerate.den;
        } else {
          _framerate = 25;
        }

        if (ret == 0) {
          _width = _frame->width;
          _height = _frame->height;

//          __android_log_print(ANDROID_LOG_WARN, "SDL_UpdateYUVTexture", "ret=%d", ret);

          //SDL_UnlockTexture(sdl_texture);

          if (_isRGB) {
            PrintTimer timer("cpu");
            DisplayYUV_16((unsigned int *) outData, _frame->data[0], _frame->data[1],
                          _frame->data[2],
                          _codecContext->width, _codecContext->height, _frame->linesize[0],
                          _frame->linesize[1], _codecContext->width);
          } else {
//            int yuvLen = _codecContext->width * _codecContext->height * 3 / 2;
//            unsigned char *yuvData = new unsigned char[yuvLen];
//            pgm_save2(_frame->data[0], _frame->linesize[0], _codecContext->width,
//                      _codecContext->height, (uint8_t*)outData);
//            pgm_save2(_frame->data[1], _frame->linesize[1], _codecContext->width / 2,
//                      _codecContext->height / 2,
//                      (uint8_t*)outData + _codecContext->width * _codecContext->height);
//            pgm_save2(_frame->data[2], _frame->linesize[2], _codecContext->width / 2,
//                      _codecContext->height / 2,
//                      (uint8_t*)outData + _codecContext->width * _codecContext->height * 5 / 4);
            int width = _frame->width;
            int height = _frame->height;
            {


              if (_surface == 0) {

                const EGLint pi32ConfigAttribs[] =
                    {
                        EGL_LEVEL, 0,
//                        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
                        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                        EGL_NATIVE_RENDERABLE, EGL_FALSE,
                        EGL_DEPTH_SIZE, EGL_DONT_CARE,
//                      EGL_RED_SIZE,   16,
//                      EGL_GREEN_SIZE, 16,
//                      EGL_BLUE_SIZE,  16,
//                      EGL_ALPHA_SIZE, 16,
                        EGL_NONE
                    };

                const EGLint param[] = {
                    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                    EGL_NONE
                };

                EGLint iConfigs;
                eglChooseConfig(_display, param, nullptr, 0, &iConfigs);
                int a[3] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

                __android_log_print(ANDROID_LOG_WARN, "GLES", "iConfigs=%d",
                                    iConfigs);

                EGLConfig config;
                eglChooseConfig(_display, param, &config, 1, &iConfigs);

                EGLint eglMajVers, eglMinVers;
                bool initialize = eglInitialize(_display, &eglMajVers, &eglMinVers);
                __android_log_print(ANDROID_LOG_WARN, "GLES", "initialize=%d", initialize);
                __android_log_print(ANDROID_LOG_WARN, "aaa", "EGL init with version %d.%d",
                                    eglMajVers, eglMinVers);

                const EGLint surfaceAttr[] = {
                    EGL_WIDTH, width,
                    EGL_HEIGHT, height,
                    EGL_NONE
                };
//                _surface = eglCreatePbufferSurface(_display, config, surfaceAttr);

                __android_log_print(ANDROID_LOG_WARN, "GLES", "_nativeWindowType=%d",
                                    _nativeWindowType);
                _surface = eglCreateWindowSurface(_display, config, _nativeWindowType, nullptr);
                __android_log_print(ANDROID_LOG_WARN, "GLES", "_surface=%d", _surface);
                //EGLSurface surface = eglCreateWindowSurface(display, config, nullptr, nullptr);

                EGLContext context = eglCreateContext(_display, config, EGL_NO_CONTEXT, a);
                EGLBoolean ret = eglMakeCurrent(_display, _surface, _surface, context);
                __android_log_print(ANDROID_LOG_WARN, "GLES", "ret=%d", ret);
                // 初始化opengles2.0
                _program = CreateProgram();
                __android_log_print(ANDROID_LOG_WARN, "GLES", "program=%d", _program);

                _position = glGetAttribLocation(_program, "aPosition");
                __android_log_print(ANDROID_LOG_WARN, "GLES", "_position=%d", _position);
                _coord = glGetAttribLocation(_program, "aTexCoord");
                __android_log_print(ANDROID_LOG_WARN, "GLES", "_coord=%d", _coord);
                _y = glGetUniformLocation(_program, "Ytex");
                __android_log_print(ANDROID_LOG_WARN, "GLES", "_y=%d", _y);
                _u = glGetUniformLocation(_program, "Utex");
                __android_log_print(ANDROID_LOG_WARN, "GLES", "_u=%d", _u);
                _v = glGetUniformLocation(_program, "Vtex");
                __android_log_print(ANDROID_LOG_WARN, "GLES", "_v=%d", _v);

//                glGenFramebuffers(1, &_fbo);
//                glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
//                __android_log_print(ANDROID_LOG_WARN, "GLES", "_fbo=%d", _fbo);

//                glGenRenderbuffers(1, &_rb);
//                __android_log_print(ANDROID_LOG_WARN, "GLES", "_rb=%d", _rb);
//                // 分配存储空间
//                glBindRenderbuffer(GL_RENDERBUFFER, _rb);
//                glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_ATTACHMENT, width, height);
//                glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _rb);
//                glBindRenderbuffer(GL_RENDERBUFFER, 0);

//                glGenTextures(1, &_colorBuffer);
//                glBindTexture(GL_TEXTURE_2D, _colorBuffer);
//                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5,
//                             nullptr);
//                glBindTexture(GL_TEXTURE_2D, 0);
//                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _colorBuffer, 0);
//
//                glBindFramebuffer(GL_FRAMEBUFFER, 0);
//
//                GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
//                if (status != GL_FRAMEBUFFER_COMPLETE) {
//                  __android_log_print(ANDROID_LOG_WARN, "glCheckFramebufferStatus", "glCheckFramebufferStatus failed status=%d", status);
//                }
              }

              if (_texYId <= 0) {
                glGenTextures(1, &_texYId);
              }

//            __android_log_print(ANDROID_LOG_WARN, "_texYId", "_texYId=%d", _texYId);
              glBindTexture(GL_TEXTURE_2D, _texYId);
              glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE,
                           GL_UNSIGNED_BYTE, _frame->data[0]);
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//            glBindTexture(GL_TEXTURE_2D, 0);

              if (_texUId <= 0) {
                glGenTextures(1, &_texUId);
              }

              glBindTexture(GL_TEXTURE_2D, _texUId);
//            __android_log_print(ANDROID_LOG_WARN, "_texUId", "_texUId=%d", _texUId);
              glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width / 2, height / 2, 0, GL_LUMINANCE,
                           GL_UNSIGNED_BYTE, _frame->data[1]);
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//              glBindTexture(GL_TEXTURE_2D, 0);

              if (_texVId <= 0) {
                glGenTextures(1, &_texVId);
              }
              glBindTexture(GL_TEXTURE_2D, _texVId);
//            __android_log_print(ANDROID_LOG_WARN, "_texVId", "_texVId=%d", _texVId);
              glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width / 2, height / 2, 0, GL_LUMINANCE,
                           GL_UNSIGNED_BYTE, _frame->data[2]);
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//              glBindTexture(GL_TEXTURE_2D, 0);

              glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
              glClear(GL_COLOR_BUFFER_BIT);

              glUseProgram(_program);

              glEnableVertexAttribArray(_position);
              glVertexAttribPointer(_position, 2, GL_FLOAT, GL_FALSE, 8, vertice_buffer);
              glEnableVertexAttribArray(_coord);
              glVertexAttribPointer(_coord, 2, GL_FLOAT, GL_FALSE, 8, fragment_buffer);

              // bind textures
              glActiveTexture(GL_TEXTURE0);
              glBindTexture(GL_TEXTURE_2D, _texYId);
              glUniform1i(_y, 0);
//            glBindTexture(GL_TEXTURE_2D, 0);

              glActiveTexture(GL_TEXTURE1);
              glBindTexture(GL_TEXTURE_2D, _texUId);
              glUniform1i(_u, 1);
//            glBindTexture(GL_TEXTURE_2D, 0);

              glActiveTexture(GL_TEXTURE2);
              glBindTexture(GL_TEXTURE_2D, _texVId);
              glUniform1i(_v, 2);
//            glBindTexture(GL_TEXTURE_2D, 0);


//              glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _texYId, 0);
//              glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _texUId, 1);
//              glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _texVId, 2);


//            glBindFramebuffer(GL_FRAMEBUFFER, 0);

//            glDrawArrays(GL_TRIANGLES,0,4);
              glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
              eglSwapBuffers(_display, _surface);
//              glFlush();
//              glFinish();

//            glBindFramebuffer(GL_FRAMEBUFFER, _vbo);
//            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            }

//            PrintTimer atimer("glReadPixels");
//            glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, outData);

//            glDisableVertexAttribArray(_position);
//            glDisableVertexAttribArray(_coord);
//
//            glReadPixels();
//            glUseProgram(0);
//            delete[] yuvData;
//            glBindFramebuffer(GL_FRAMEBUFFER, 0);

          }

          outLen = _frame->width * _frame->height * 2;
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

  GLuint Decoder::CreateShader(int shaderType, const char *source) {
    // 创建shader对象
    GLuint shader = glCreateShader(shaderType);
    // 把shader代码上传到gpu
    glShaderSource(shader, 1, &source, nullptr);
    // 编译shader
    glCompileShader(shader);
    __android_log_print(ANDROID_LOG_WARN, "GLES", "shader=%d", shader);
    return shader;
  }

  // gpu程序
  GLuint Decoder::CreateProgram() {
    GLuint vShader = CreateShader(GL_VERTEX_SHADER, vertexShader);
    GLuint tShader = CreateShader(GL_FRAGMENT_SHADER, fragmentShader_yuv420p);
    GLuint program = glCreateProgram();
    // shader跟程序绑定
    glAttachShader(program, vShader);
    glAttachShader(program, tShader);
    // 链接程序
    glLinkProgram(program);
    // 解除绑定
    glDetachShader(program, vShader);
    glDetachShader(program, tShader);
    // 删除shader
    glDeleteShader(vShader);
    glDeleteShader(tShader);

    return program;
  }


}