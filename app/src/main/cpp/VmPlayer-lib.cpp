#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include "plateform/VmPlayer.h"

extern "C" {
}

#define TAG "VmPlayer-jni" // 这个是自定义的LOG的标识
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__) // 定义LOGD类型
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__) // 定义LOGI类型
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__) // 定义LOGW类型
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__) // 定义LOGE类型
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL, TAG, __VA_ARGS__) // 定义LOGF类型

const char *METHOD_NAME_FRAMECONFHOLDER_INIT = "init";
const char *METHOD_SIG_FRAMECONFHOLDER_INIT = "(III)V";

// 接口函数-------------------------------------------------------------------------
extern "C" {

bool FrameConfigHolderInit(JNIEnv *env, int width, int height, int framerate,
                           jobject frameConfigHolder) {
    jclass frameConfigCls = env->GetObjectClass(frameConfigHolder);

    jmethodID init = env->GetMethodID(frameConfigCls, METHOD_NAME_FRAMECONFHOLDER_INIT,
                                      METHOD_SIG_FRAMECONFHOLDER_INIT);
    if (init == nullptr) {
        LOGE("找不到[%s]方法!", METHOD_NAME_FRAMECONFHOLDER_INIT);
        env->DeleteLocalRef(frameConfigCls);
        return false;
    }

    env->CallVoidMethod(frameConfigHolder, init, width, height, framerate);
    env->DeleteLocalRef(frameConfigCls);
    return true;
}

jlong Java_com_joyware_vmsdk_core_Decoder_DecoderInit(JNIEnv *env, jobject ob,
                                                      jint payloadType) {
    long decoderHandle = 0;
    LOGI("Java_com_joyware_vmsdk_VmPlayer_DecoderInit(%d)", payloadType);
    bool ret = VmPlayer_DecoderInit(payloadType, decoderHandle);
    if (!ret) {
        decoderHandle = 0;
    }
    return decoderHandle;
}

void Java_com_joyware_vmsdk_core_Decoder_DecoderUninit(JNIEnv *env, jobject ob,
                                                       jlong decoderHandle) {
    LOGI("Java_com_joyware_vmsdk_VmPlayer_DecoderUninit(%lld)", decoderHandle);
    VmPlayer_DecoderUninit(decoderHandle);
}

jint Java_com_joyware_vmsdk_core_Decoder_DecodeNalu2RGB(JNIEnv *env, jobject ob,
                                                        jlong decoderHandle,
                                                        jbyteArray inData, jint inLen,
                                                        jbyteArray outData,
                                                        jobject frameConfigHolder) {
//  LOGE("Java_com_joyware_vmsdk_VmPlayer_DecodeNalu(%d)", outData);

    jbyte *inBuf = env->GetByteArrayElements(inData, 0);
    jbyte *outBuf = outData != nullptr ? env->GetByteArrayElements(outData, 0) : nullptr;

    int outlen = outData != nullptr ? env->GetArrayLength(outData) : 0;

    int width = 0;
    int height = 0;
    int framerate = 0;
    bool ret = VmPlayer_DecodeNalu2RGB(decoderHandle, (const char *) inBuf, inLen, (char *) outBuf,
                                       outlen, width, height, framerate);
    if (ret) {
        if (!FrameConfigHolderInit(env, width, height, framerate, frameConfigHolder)) {
            outlen = -1;
        }
    } else {
        outlen = -1;
    }

    env->ReleaseByteArrayElements(inData, inBuf, 0);

    if (outBuf != nullptr) {
        env->ReleaseByteArrayElements(outData, outBuf, 0);
    }

    return outlen;
}

jint Java_com_joyware_vmsdk_core_Decoder_DecodeNalu2YUV(JNIEnv *env, jobject ob,
                                                        jlong decoderHandle,
                                                        jbyteArray inData, jint inLen,
                                                        jbyteArray yData, jbyteArray uData,
                                                        jbyteArray vData,
                                                        jobject frameConfigHolder) {
    //LOGI("Java_com_joyware_vmsdk_VmPlayer_DecodeNalu(%d)", payloadType);

    jbyte *inBuf = env->GetByteArrayElements(inData, 0);
    jbyte *yBuf = env->GetByteArrayElements(yData, 0);
    jbyte *uBuf = env->GetByteArrayElements(uData, 0);
    jbyte *vBuf = env->GetByteArrayElements(vData, 0);

    int outlen = -1;
    int ylen = env->GetArrayLength(yData);
    int ulen = env->GetArrayLength(uData);
    int vlen = env->GetArrayLength(vData);
    int width = 0;
    int height = 0;
    int framerate = 0;
    bool ret = VmPlayer_DecodeNalu2YUV(decoderHandle, (const char *) inBuf, inLen, (char *) yBuf,
                                       ylen,
                                       (char *) uBuf, ulen, (char *) vBuf, vlen, width, height,
                                       framerate);
    if (ret) {
        if (!FrameConfigHolderInit(env, width, height, framerate, frameConfigHolder)) {
            return false;
        }
        outlen = width * height * 3 / 2;
    } else {
        outlen = -1;
    }

    env->ReleaseByteArrayElements(inData, inBuf, 0);
    env->ReleaseByteArrayElements(yData, yBuf, 0);
    env->ReleaseByteArrayElements(uData, uBuf, 0);
    env->ReleaseByteArrayElements(vData, vBuf, 0);

    return outlen;
}

jint Java_com_joyware_vmsdk_core_Decoder_GetFrameWidth(JNIEnv *env, jobject ob,
                                                       jlong decoderHandle) {
    LOGI("Java_com_joyware_vmsdk_core_Decoder_GetFrameWidth(%lld)", decoderHandle);
    return VmPlayer_GetFrameWidth(decoderHandle);
}

jint Java_com_joyware_vmsdk_core_Decoder_GetFrameHeight(JNIEnv *env, jobject ob,
                                                        jlong decoderHandle) {
    LOGI("Java_com_joyware_vmsdk_core_Decoder_GetFrameHeight(%lld)", decoderHandle);
    return VmPlayer_GetFrameHeight(decoderHandle);
}

jlong Java_com_joyware_vmsdk_core_Decoder_AACEncoderInit(JNIEnv *env, jobject ob,
                                                         jint samplerate) {
    long encoderHandle = 0;
    LOGI("Java_com_joyware_vmsdk_core_Decoder_AACEncoderInit(%d)", samplerate);
    bool ret = VmPlayer_ACCEncoderInit(samplerate, encoderHandle);
    if (!ret) {
        encoderHandle = 0;
    }
    return encoderHandle;
}

void Java_com_joyware_vmsdk_core_Decoder_AACEncoderUninit(JNIEnv *env, jobject ob,
                                                          jlong encoderHandle) {
    LOGI("Java_com_joyware_vmsdk_core_Decoder_AACEncoderUninit(%lld)", encoderHandle);
    VmPlayer_ACCEncoderUninit(encoderHandle);
}

jint Java_com_joyware_vmsdk_core_Decoder_AACEncodePCM2AAC(JNIEnv *env, jobject ob,
                                                          jlong encoderHandle,
                                                          jbyteArray inData, jint inLen,
                                                          jbyteArray outData) {

    jbyte *inBuf = env->GetByteArrayElements(inData, 0);
    jbyte *outBuf = outData != nullptr ? env->GetByteArrayElements(outData, 0) : nullptr;

    int outlen = outData != nullptr ? env->GetArrayLength(outData) : 0;

    bool ret = VmPlayer_AACEncodePCM2AAC(encoderHandle, (const char *) inBuf, inLen,
                                         (char *) outBuf,
                                         outlen);

    if (!ret) {
        outlen = -1;
    }

    env->ReleaseByteArrayElements(inData, inBuf, 0);

    if (outBuf != nullptr) {
        env->ReleaseByteArrayElements(outData, outBuf, 0);
    }

    return outlen;
}

jlong Java_com_joyware_vmsdk_core_Decoder_RenderInit(JNIEnv *env, jobject ob,
                                                     jobject surface) {
    long renderHandle = 0;
    LOGW("Java_com_joyware_vmsdk_core_Decoder_RenderInit()");
    ANativeWindow *pNativeWindow = ANativeWindow_fromSurface(env, surface);
    bool ret = VmPlayer_RenderInit((void *) pNativeWindow, renderHandle);
//  ANativeWindow_release(pNativeWindow);
    if (!ret) {
        renderHandle = 0;
    }
    return renderHandle;
}

void Java_com_joyware_vmsdk_core_Decoder_RenderUninit(JNIEnv *env, jobject ob,
                                                       jlong renderHandle) {
    LOGW("Java_com_joyware_vmsdk_core_Decoder_RenderUninit(%lld)", renderHandle);
    VmPlayer_RenderUninit((long) renderHandle);
}

void Java_com_joyware_vmsdk_core_Decoder_RenderSurfaceCreated(JNIEnv *env, jobject ob,
                                                      jlong renderHandle) {
    LOGW("Java_com_joyware_vmsdk_core_Decoder_RenderSurfaceCreated(%lld)", renderHandle);
    VmPlayer_RenderSurfaceCreated((long) renderHandle);
}

void Java_com_joyware_vmsdk_core_Decoder_RenderSurfaceDestroyed(JNIEnv *env, jobject ob,
                                                      jlong renderHandle) {
    LOGW("Java_com_joyware_vmsdk_core_Decoder_RenderSurfaceDestroyed(%lld)", renderHandle);
    VmPlayer_RenderSurfaceDestroyed((long) renderHandle);
}

void
Java_com_joyware_vmsdk_core_Decoder_RenderSurfaceChanged(JNIEnv *env, jobject ob, jlong renderHandle,
                                                     jint width, jint height) {
    VmPlayer_RenderSurfaceChanged((long) renderHandle, width, height);
}

void Java_com_joyware_vmsdk_core_Decoder_RenderScaleTo(JNIEnv *env, jobject ob, jlong renderHandle,
                                                       jboolean enable, jint centerX, jint centerY,
                                                       jfloat widthScale, jfloat heightScale) {
    VmPlayer_RenderScaleTo((long) renderHandle, enable, centerX, centerY, widthScale, heightScale);
}

jint Java_com_joyware_vmsdk_core_Decoder_DrawYUV(JNIEnv *env, jobject ob, jlong renderHandle,
                                                 jbyteArray yData, jint yStart, jint yLen,
                                                 jbyteArray uData, jint uStart, jint uLen,
                                                 jbyteArray vData, jint vStart, jint vLen,
                                                 jint width, jint height) {
    //LOGI("Java_com_joyware_vmsdk_VmPlayer_DecodeNalu(%d)", payloadType);

    jbyte *yBuf = env->GetByteArrayElements(yData, 0);
    jbyte *uBuf = env->GetByteArrayElements(uData, 0);
    jbyte *vBuf = env->GetByteArrayElements(vData, 0);

    int outlen = -1;
    bool ret = VmPlayer_RenderDrawYUV((long) renderHandle, (const char *) (yBuf + yStart), yLen,
                                      (const char *) (uBuf + uStart), uLen,
                                      (const char *) (vBuf + vStart), vLen, width, height);
    if (ret) {
        outlen = 0;
    } else {
        outlen = -1;
    }

    env->ReleaseByteArrayElements(yData, yBuf, 0);
    env->ReleaseByteArrayElements(uData, uBuf, 0);
    env->ReleaseByteArrayElements(vData, vBuf, 0);

    return outlen;
}

jint Java_com_joyware_vmsdk_core_Decoder_OfflineScreenRendering(JNIEnv *env, jobject ob,
                                                                jlong renderHandle,
                                                                jbyteArray yData, jint yLen,
                                                                jbyteArray uData,
                                                                jint uLen, jbyteArray vData,
                                                                jint vLen, jint width, jint height,
                                                                jbyteArray outData) {
    //LOGI("Java_com_joyware_vmsdk_VmPlayer_DecodeNalu(%d)", payloadType);

    jbyte *yBuf = env->GetByteArrayElements(yData, 0);
    jbyte *uBuf = env->GetByteArrayElements(uData, 0);
    jbyte *vBuf = env->GetByteArrayElements(vData, 0);
    jbyte *outBuf = env->GetByteArrayElements(outData, 0);

    int outlen = env->GetArrayLength(outData);
    bool ret = VmPlayer_RenderOffScreenRendering(renderHandle, (const char *) yBuf, yLen,
                                                 (const char *) uBuf, uLen, (const char *) vBuf,
                                                 vLen,
                                                 width, height, (char *) outBuf, outlen);
    if (!ret) {
        outlen = -1;
    }

    env->ReleaseByteArrayElements(yData, yBuf, 0);
    env->ReleaseByteArrayElements(uData, uBuf, 0);
    env->ReleaseByteArrayElements(vData, vBuf, 0);
    env->ReleaseByteArrayElements(outData, outBuf, 0);

    return outlen;
}

}