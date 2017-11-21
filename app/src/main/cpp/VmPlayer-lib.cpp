#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include "plateform/VmPlayer.h"
#include "cipher/jw_cipher.h"

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

//    LOGE("VmPlayer_DecodeNalu2RGB(%d)", ret);
    if (ret) {
        if (!FrameConfigHolderInit(env, width, height, framerate, frameConfigHolder)) {

        }
        outlen = width * height * 2;
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
                                       ylen, (char *) uBuf, ulen, (char *) vBuf, vlen, width,
                                       height, framerate);
    if (ret) {
        if (!FrameConfigHolderInit(env, width, height, framerate, frameConfigHolder)) {

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

    bool ret = VmPlayer_AACEncodePCM2AAC((long) encoderHandle, (const char *) inBuf, inLen,
                                         (char *) outBuf, outlen);

    if (!ret) {
        outlen = -1;
    }

    env->ReleaseByteArrayElements(inData, inBuf, 0);

    if (outBuf != nullptr) {
        env->ReleaseByteArrayElements(outData, outBuf, 0);
    }

    return outlen;
}

jlong Java_com_joyware_vmsdk_core_GLHelper_nativeInit(JNIEnv *env, jobject ob) {
    long renderHandle = 0;
    LOGW("Java_com_joyware_vmsdk_core_GLHelper_nativeInit()");
    bool ret = VmPlayer_RenderInit(renderHandle);
//  ANativeWindow_release(pNativeWindow);
    if (!ret) {
        renderHandle = 0;
    }
    return renderHandle;
}

void Java_com_joyware_vmsdk_core_GLHelper_nativeUninit(JNIEnv *env, jobject ob,
                                                       jlong renderHandle) {
    LOGW("Java_com_joyware_vmsdk_core_GLHelper_nativeUninit(%lld)", renderHandle);
    VmPlayer_RenderUninit((long) renderHandle);
}

jlong
Java_com_joyware_vmsdk_core_GLHelper_nativeStart(JNIEnv *env, jobject ob, jlong renderHandle) {
    LOGW("Java_com_joyware_vmsdk_core_GLHelper_nativeStart(%lld)", renderHandle);
    return VmPlayer_RenderStart((long) renderHandle);
}

void Java_com_joyware_vmsdk_core_GLHelper_nativeFinish(JNIEnv *env, jobject ob,
                                                       jlong renderHandle) {
    LOGW("Java_com_joyware_vmsdk_core_GLHelper_nativeFinish(%lld)", renderHandle);
    VmPlayer_RenderFinish((long) renderHandle);
}

jlong Java_com_joyware_vmsdk_core_GLHelper_nativeCreateSurface(JNIEnv *env, jobject ob,
                                                               jlong renderHandle,
                                                               jobject surface) {
    LOGW("Java_com_joyware_vmsdk_core_GLHelper_nativeCreateSurface(%lld)", renderHandle);
    ANativeWindow *pNativeWindow = ANativeWindow_fromSurface(env, surface);
    return VmPlayer_RenderCreateSurface((long) renderHandle, (void *) pNativeWindow);
}

void Java_com_joyware_vmsdk_core_GLHelper_nativeDestroySurface(JNIEnv *env, jobject ob,
                                                               jlong renderHandle) {
    LOGW("Java_com_joyware_vmsdk_core_GLHelper_nativeDestroySurface(%lld)", renderHandle);
    VmPlayer_RenderDestroySurface((long) renderHandle);
}

void Java_com_joyware_vmsdk_core_GLHelper_nativeSurfaceCreated(JNIEnv *env, jobject ob,
                                                               jlong renderHandle) {
    LOGW("Java_com_joyware_vmsdk_core_GLHelper_nativeSurfaceCreated(%lld)", renderHandle);
    VmPlayer_RenderSurfaceCreated((long) renderHandle);
}

void Java_com_joyware_vmsdk_core_GLHelper_nativeSurfaceDestroyed(JNIEnv *env, jobject ob,
                                                                 jlong renderHandle) {
    LOGW("Java_com_joyware_vmsdk_core_GLHelper_nativeSurfaceDestroyed(%lld)", renderHandle);
    VmPlayer_RenderSurfaceDestroyed((long) renderHandle);
}

void
Java_com_joyware_vmsdk_core_GLHelper_nativeSurfaceChanged(JNIEnv *env, jobject ob,
                                                          jlong renderHandle,
                                                          jint width, jint height) {
//    LOGW("Java_com_joyware_vmsdk_core_GLHelper_nativeSurfaceChanged(%lld, %d, %d)", renderHandle,
//         width, height);
    VmPlayer_RenderSurfaceChanged((long) renderHandle, width, height);
}

void Java_com_joyware_vmsdk_core_GLHelper_nativeScaleTo(JNIEnv *env, jobject ob, jlong renderHandle,
                                                        jboolean enable, jint left, jint top,
                                                        jint width, jint height) {
//    LOGW("Java_com_joyware_vmsdk_core_GLHelper_nativeScaleTo(%lld)", renderHandle);
    VmPlayer_RenderScaleTo((long) renderHandle, enable, left, top, width, height);
}

jint Java_com_joyware_vmsdk_core_GLHelper_nativeDrawYUV(JNIEnv *env, jobject ob, jlong renderHandle,
                                                        jbyteArray yData, jint yStart, jint yLen,
                                                        jbyteArray uData, jint uStart, jint uLen,
                                                        jbyteArray vData, jint vStart, jint vLen,
                                                        jint width, jint height) {
    //LOGI("Java_com_joyware_vmsdk_VmPlayer_DecodeNalu(%d)", payloadType);

    jbyte *yBuf = env->GetByteArrayElements(yData, 0);
    jbyte *uBuf = env->GetByteArrayElements(uData, 0);
    jbyte *vBuf = env->GetByteArrayElements(vData, 0);

    int ret = VmPlayer_RenderDrawYUV((long) renderHandle, (const char *) (yBuf + yStart), yLen,
                                     (const char *) (uBuf + uStart), uLen,
                                     (const char *) (vBuf + vStart), vLen, width, height);

    env->ReleaseByteArrayElements(yData, yBuf, 0);
    env->ReleaseByteArrayElements(uData, uBuf, 0);
    env->ReleaseByteArrayElements(vData, vBuf, 0);

    return ret;
}

jint Java_com_joyware_vmsdk_core_GLHelper_nativeOfflineScreenRendering(JNIEnv *env, jobject ob,
                                                                       jlong renderHandle,
                                                                       jbyteArray yData, jint yLen,
                                                                       jbyteArray uData,
                                                                       jint uLen, jbyteArray vData,
                                                                       jint vLen, jint width,
                                                                       jint height,
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

bool Java_com_joyware_vmsdk_core_Decoder_YUVSP2YUVP(JNIEnv *env, jobject ob,
                                                    jbyteArray inuvData, jint inuvStart,
                                                    jint inuvTotalLen,
                                                    jbyteArray outuvData) {
    //LOGI("Java_com_joyware_vmsdk_VmPlayer_DecodeNalu(%d)", payloadType);

    jbyte *inArray = env->GetByteArrayElements(inuvData, 0);
    jbyte *outArray = env->GetByteArrayElements(outuvData, 0);

    int uLen = inuvTotalLen / 2;
    for (int i = 0; i < uLen; ++i) {
        outArray[i] = inArray[inuvStart + 2 * i];
        outArray[i + uLen] = inArray[inuvStart + 2 * i + 1];
    }


    env->ReleaseByteArrayElements(inuvData, inArray, 0);
    env->ReleaseByteArrayElements(outuvData, outArray, 0);

    return true;
}

// NV21 是uv交叉存储，yyyyyyvuvuvu
bool Java_com_joyware_vmsdk_core_YUVRenderer_YUVP2NV21(JNIEnv *env, jobject ob,
                                                       jbyteArray inuvData, jint inuvStart,
                                                       jint inuvTotalLen,
                                                       jbyteArray outuvData) {
    //LOGI("Java_com_joyware_vmsdk_VmPlayer_DecodeNalu(%d)", payloadType);

    jbyte *inArray = env->GetByteArrayElements(inuvData, 0);
    jbyte *outArray = env->GetByteArrayElements(outuvData, 0);

    int uLen = inuvTotalLen / 2;
    for (int i = 0; i < uLen; ++i) {
        outArray[2 * i + 1] = inArray[inuvStart + i];
        outArray[2 * i] = inArray[inuvStart + uLen + i];
    }


    env->ReleaseByteArrayElements(inuvData, inArray, 0);
    env->ReleaseByteArrayElements(outuvData, outArray, 0);

    return true;
}

//jlong JNICALL
//Java_com_joyware_vmsdk_core_Decoder_JWCipherCreate(JNIEnv *env, jclass type, jbyteArray key_,
//                                                   jint keyLen) {
//    jbyte *key = env->GetByteArrayElements(key_, NULL);
//
//    JW_CIPHER_CTX ctx = jw_cipher_create((const unsigned char *) key, keyLen);
//
//    env->ReleaseByteArrayElements(key_, key, 0);
//
//    return (jlong) ctx;
//}
//
//void JNICALL
//Java_com_joyware_vmsdk_core_Decoder_JWCipherRelease(JNIEnv *env, jclass type, jlong cipherCtx) {
//    if (cipherCtx != NULL) {
//        jw_cipher_release((void *) cipherCtx);
//    }
//}
//
//void JNICALL
//Java_com_joyware_vmsdk_core_Decoder_JWCipherDecryptH264(JNIEnv *env, jclass type, jlong cipherCtx,
//                                                        jbyteArray h264data_, jint dataBegin,
//                                                        jint dataLen) {
//    if (h264data_ != nullptr && dataLen > 0) {
//        jbyte *h264data = env->GetByteArrayElements(h264data_, NULL);
//
//        jw_cipher_decrypt_h264((void *) cipherCtx, (unsigned char *) (h264data + dataBegin),
//                               (size_t) dataLen);
//
//        env->ReleaseByteArrayElements(h264data_, h264data, 0);
//    }
//}
//
//int JNICALL
//Java_com_joyware_vmsdk_core_Decoder_JWCipherDecrypt(JNIEnv *env, jclass type, jlong cipherCtx,
//                                                    jbyteArray in_, jint inBegin, jint inLen,
//                                                    jbyteArray out_, jint outBegin, jint outLen) {
//    if (in_ != nullptr && inLen > 0 && out_ != nullptr && outLen > 0) {
//        jbyte *in = env->GetByteArrayElements(in_, NULL);
//        jbyte *out = env->GetByteArrayElements(out_, NULL);
//
//        size_t outlen = (size_t) outLen;
//        int ret = jw_cipher_decrypt((void *) cipherCtx, (const unsigned char *) (in + inBegin),
//                                    (size_t) inLen, (unsigned char *) (out + outBegin), &outlen);
//
//        env->ReleaseByteArrayElements(in_, in, 0);
//        env->ReleaseByteArrayElements(out_, out, 0);
//
//        if (ret == 0) {
//            return -1;
//        } else {
//            return outlen;
//        }
//    }
//}

}