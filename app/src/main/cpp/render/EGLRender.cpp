//
// Created by zhou rui on 16/11/9.
//

#include "../util/public/platform.h"
#include "EGLRender.h"

namespace Dream {

  EGLRender::EGLRender() : _inited(false), _program(0), _position(-1), _coord(-1), _y(-1), _u(-1),
                           _u(-1), _texYId(0), _texUId(0), _texVId(0), _display(0), _context(0),
                           _surface(0), _nativeWindow(0) {

  }

  EGLRender::~EGLRender() {

  }

  bool EGLRender::Init(EGLNativeWindowType nativeWindow) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_inited) {
      return true;
    }

    // 获取原生窗口
    _nativeWindow = nativeWindow;

    // 获取默认显卡
    _display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (_display == EGL_NO_DISPLAY) {
      LOGE(TAG, "display EGL_NO_DISPLAY!\n");
      return false;
    }

    //  选择参数
    const EGLint chooseAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    // 配置信息
    EGLConfig config;
    // 配置个数
    EGLint iConfigs;

    // 获取一个配置
    eglChooseConfig(_display, chooseAttribs, &config, 1, &iConfigs);

    // 初始化并获取版本信息
    EGLint eglMajVers, eglMinVers;
    if (!eglInitialize(_display, &eglMajVers, &eglMinVers)) {
      LOGE(TAG, "eglInitialize failed!\n");
      releaseResources();
      return false;
    }
    LOGW(TAG, "eglInitialize success. version %d.%d\n", eglMajVers, eglMinVers);

    // 上下文参数，使用opengles2.0版本
    const EGLint contextAttribs[3] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    // 创建上下文
    _context = eglCreateContext(_display, config, EGL_NO_CONTEXT, contextAttribs);
    if (_context == EGL_NO_CONTEXT) {
      LOGE(TAG, "context EGL_NO_CONTEXT!\n");
      releaseResources();
      return false;
    }

    // 创建缓冲
    if (_nativeWindow) {
      // 直接渲染
      _surface = eglCreateWindowSurface(_display, config, _nativeWindow, nullptr);
    } else {
      // 离屏渲染
      _surface = eglCreatePbufferSurface(_display, config, nullptr);
    }
    if (_surface == EGL_NO_SURFACE) {
      LOGE(TAG, "surface EGL_NO_SURFACE!\n");
      releaseResources();
      return false;
    }

    // 渲染上下文、显卡、缓冲关联
    if (!eglMakeCurrent(_display, _surface, _surface, _context)) {
      LOGE(TAG, "eglMakeCurrent failed!\n");
      releaseResources();
      return false;
    }

    // 编译gpu程序
    _program = buildProgram();
    if (_program == 0) {
      LOGE(TAG, "BuildProgram failed!\n");
      releaseResources();
      return false;
    }

    // 获取参数id
    if (!initParamLocation()) {
      LOGE(TAG, "initParamLocation failed!\n");
      releaseResources();
      return false;
    }

    if (!initTextureId()) {
      LOGE(TAG, "initTextureId failed!\n");
      releaseResources();
      return false;
    }

    LOGW(TAG, "EGLRender init success.\n");
    _inited = true;
  }

  void EGLRender::Uninit() {
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_inited) {
      return;
    }

    releaseResources();
    _inited = false;
  }

  bool EGLRender::DrawYUV(const char *yData, int yLen, const char *uData, int uLen,
                          const char *vData, int vLen, int width, int height) {
    if (!_inited || _nativeWindow == nullptr) {
      return false;
    }

    bool execute = executeProgram(yData, yLen, uData, uLen, vData, vLen, width, height);
    if (execute) {
      eglSwapBuffers(_display, _surface);
    }
    return execute;
  }

  bool EGLRender::OffScreenRendering(const char *yData, int yLen, const char *uData, int uLen, const char *vData,
                          int vLen, int width, int height, char* outRgbData, int& outRgbLen) {
    if (!_inited || _nativeWindow != nullptr) {
      return false;
    }

    bool execute = executeProgram(yData, yLen, uData, uLen, vData, vLen, width, height);
    if (execute) {
      glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, outRgbData);
      outRgbLen = width*height*2;
    }
    return execute;
  }

  bool EGLRender::executeProgram(const char *yData, int yLen, const char *uData, int uLen, const char *vData,
                      int vLen, int width, int height) {
    // 绑定纹理
    glBindTexture(GL_TEXTURE_2D, _texYId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE,
                 GL_UNSIGNED_BYTE, yData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


    glBindTexture(GL_TEXTURE_2D, _texUId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width / 2, height / 2, 0, GL_LUMINANCE,
                 GL_UNSIGNED_BYTE, uData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, _texVId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width / 2, height / 2, 0, GL_LUMINANCE,
                 GL_UNSIGNED_BYTE, vData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 清屏，由于后一帧会完全覆盖前一帧，所以这里不需要
//    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
//    glClear(GL_COLOR_BUFFER_BIT);

    // 开始执行程序
    glUseProgram(_program);

    // 传参
    glEnableVertexAttribArray(_position);
    glVertexAttribPointer(_position, 2, GL_FLOAT, GL_FALSE, 8, BUFFER_VERTICE);
    glEnableVertexAttribArray(_coord);
    glVertexAttribPointer(_coord, 2, GL_FLOAT, GL_FALSE, 8, BUFFER_FRAGMENT);

    // 绑定纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _texYId);
    glUniform1i(_y, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _texUId);
    glUniform1i(_u, 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, _texVId);
    glUniform1i(_v, 2);

    // 执行完毕
    glUseProgram(0);

    // 绘图
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }

  GLuint EGLRender::loadShader(int shaderType, const char *source) {
    GLuint shader = glCreateShader(shaderType);
    // 把shader代码上传到gpu
    glShaderSource(shader, 1, &source, nullptr);
    EGLint error = glGetError();
    if (error != GL_NO_ERROR) {
      LOGE(TAG, "glShaderSource failed, error=%d\n", error);
      return 0;
    }
    return shader;
  }

  // 编译gpu程序
  GLuint EGLRender::buildProgram() {
    GLuint vShader = loadShader(GL_VERTEX_SHADER, SHADER_VERTEX);
    GLuint tShader = loadShader(GL_FRAGMENT_SHADER, SHADER_FRAGMENT_YUV420P);
    if (vShader == 0 || tShader == 0) {
      return 0;
    }
    // 编译shader
    glCompileShader(vShader);
    glCompileShader(tShader);

    EGLint error = glGetError();
    if (error != GL_NO_ERROR) {
      LOGE(TAG, "glCompileShader failed, error=%d\n", error);
      return 0;
    }

    GLuint program = glCreateProgram();

    // shader跟程序绑定
    glAttachShader(program, vShader);
    glAttachShader(program, tShader);
    error = glGetError();
    if (error != GL_NO_ERROR) {
      LOGE(TAG, "glAttachShader failed, error=%d\n", error);
      return 0;
    }

    // 链接程序
    glLinkProgram(program);
    error = glGetError();
    if (error != GL_NO_ERROR) {
      LOGE(TAG, "glLinkProgram failed, error=%d\n", error);
      return 0;
    }

    // 解除绑定
    glDetachShader(program, vShader);
    glDetachShader(program, tShader);

    // 删除shader
    glDeleteShader(vShader);
    glDeleteShader(tShader);

    return program;
  }

  // 初始化参数Id
  bool EGLRender::initParamLocation() {
    _position = glGetAttribLocation(_program, "aPosition");
    if (_position < 0) {
      LOGE(TAG, "glGetAttribLocation aPosition failed!\n");
      return false;
    }
    _coord = glGetAttribLocation(_program, "aTexCoord");
    if (_position < 0) {
      LOGE(TAG, "glGetAttribLocation aTexCoord failed!\n");
      return false;
    }

    _y = glGetUniformLocation(_program, "Ytex");
    if (_position < 0) {
      LOGE(TAG, "glGetUniformLocation Ytex failed!\n");
      return false;
    }
    _u = glGetUniformLocation(_program, "Utex");
    if (_position < 0) {
      LOGE(TAG, "glGetUniformLocation Utex failed!\n");
      return false;
    }
    _v = glGetUniformLocation(_program, "Vtex");
    if (_position < 0) {
      LOGE(TAG, "glGetUniformLocation Vtex failed!\n");
      return false;
    }

    return true;
  }

  bool EGLRender::initTextureId() {
    if (_texYId == 0) {
      glGenTextures(1, &_texYId);
    }
    if (_texUId == 0) {
      glGenTextures(1, &_texUId);
    }
    if (_texVId == 0) {
      glGenTextures(1, &_texVId);
    }
    if (_texYId == 0 || _texUId == 0 || _texVId == 0) {
      LOGE(TAG, "glGenTextures failed!\n");
      return false;
    }
    return true;
  }

  // 释放资源
  void EGLRender::releaseResources() {
    if (_surface != EGL_NO_SURFACE) {
      eglDestroySurface(_display, _surface);
    }
    if (_context != EGL_NO_CONTEXT) {
      eglDestroyContext(_display, _context);
    }
  }
}