//
// Created by zhou rui on 16/11/9.
//

#ifndef EGLRANDER_H
#define EGLRANDER_H

#include <mutex>

#include "GLES2/gl2.h"
#include "EGL/egl.h"


/**
 * 基于opengles2.0的渲染器类，使用该类时需要链接android动态库
 * 该类接受一个nativewindow，可以直接在c++层渲染yuv数据到屏幕上
 * 支持离屏渲染
 */

namespace Dream {
  class EGLRender {
    const char* TAG = "EGLRender";

    const char* SHADER_VERTEX = {
        "attribute vec4 aPosition;\n"
            " attribute vec2 aTexCoord;\n"
            " varying vec2 vTextureCoord;\n"
            " void main() {\n"
            " gl_Position = aPosition;\n"
            " vTextureCoord = aTexCoord;\n"
            "}\n"
    };

    const char* SHADER_FRAGMENT_YUV420P = {
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

    const float BUFFER_VERTICE[8] = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        -1.0f, 1.0f,
        1.0f, 1.0f
    };
    const float BUFFER_FRAGMENT[8] = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f
    };

  public:
    EGLRender();

    ~EGLRender();

    bool Init(void *nativeWindow);

    void Uninit();

    /**
     * 绘图，必须传入不为空的nativeWindow
     */
    bool DrawYUV(const char *yData, int yLen, const char *uData, int uLen, const char *vData,
                 int vLen, int width, int height);

    /**
     * 离屏渲染，返回rgb格式数据，初始化是传入nativeWindow必须是空
     */
    bool OffScreenRendering(const char *yData, int yLen, const char *uData, int uLen, const char *vData,
                            int vLen, int width, int height, char* outRgbData, int& outRgbLen);

  private:
    // 执行程序
    bool executeProgram(const char *yData, int yLen, const char *uData, int uLen, const char *vData,
                        int vLen, int width, int height);

    // 加载shader
    GLuint loadShader(int shaderType, const char *source);

    // 编译gpu程序
    GLuint buildProgram();

    // 初始化参数Id
    bool initParamLocation();

    // 初始化纹理id
    bool initTextureId();

    // 释放资源
    void releaseResources();

  private:
    bool _inited;

    // gpu传入参数
    GLuint _program;
    GLint _position;
    GLint _coord;
    GLint _y;
    GLint _u;
    GLint _v;

    // 生成yuv纹理
    GLuint _texYId;
    GLuint _texUId;
    GLuint _texVId;

    // 初始化相关
    EGLDisplay _display;
    EGLContext _context;
    EGLSurface _surface;

    // 原生窗口指针
    EGLNativeWindowType _nativeWindow;

    std::mutex _mutex;
  };
}


#endif //VMSDKDEMO_ANDROID_EGLRANDER_H
