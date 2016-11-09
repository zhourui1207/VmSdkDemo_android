/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <Android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <jni.h>

#include "plateform/VmPlayer.h"

extern "C" {
}

#define TAG "VmPlayer-jni" // 这个是自定义的LOG的标识
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__) // 定义LOGD类型
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__) // 定义LOGI类型
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__) // 定义LOGW类型
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__) // 定义LOGE类型
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL, TAG, __VA_ARGS__) // 定义LOGF类型

//====================================================

//jint Java_org_FFmpeg_ffmpeg_Init(JNIEnv* env, jobject thiz, jint index, jint width, jint height)
//{
//  iWidth[index] = width;
//  iHeight[index] = height;
//
//  CreateYUVTab_16(index);
//
//  c[index] = avcodec_alloc_context();
//
//  avcodec_open(c[index]);
//
//  picture[index] = avcodec_alloc_frame();//picture= malloc(sizeof(AVFrame));
//
//  return 1;
//
//}
//
//jint Java_org_FFmpeg_ffmpeg_Destroy(JNIEnv* env, jobject thiz, jint index)
//{
//  if(c[index])
//  {
//    decode_end(c[index]);
//    free((c[index])->priv_data);
//
//    free(c[index]);
//    c[index] = NULL;
//  }
//
//  if(picture[index])
//  {
//    free(picture[index]);
//    picture[index] = NULL;
//  }
//
//  DeleteYUVTab(index);
//
//  return 1;
//}

//jint Java_org_FFmpeg_ffmpeg_DecoderNal(JNIEnv* env, jobject thiz, jint index, jbyteArray in, jint nalLen, jbyteArray out)
//{
//  int i;
//  int imod;
//  int got_picture;
//
//  jbyte * Buf = (jbyte*)(*env)->GetByteArrayElements(env, in, 0);
//  jbyte * Pixel= (jbyte*)(*env)->GetByteArrayElements(env, out, 0);
//
//  int consumed_bytes= decode_frame(c[index], picture[index], &got_picture, Buf, nalLen);
//
////	int consumed_bytes = h264_decode_frame(c[index], picture[index], &got_picture, Buf, nalLen);
//
//  if(consumed_bytes > 0)
//  {
//    DisplayYUV_16(index, (int*)Pixel, picture[index]->data[0], picture[index]->data[1], picture[index]->data[2], c[index]->width, c[index]->height, picture[index]->linesize[0], picture[index]->linesize[1], iWidth[index]);
//  }
//
//  (*env)->ReleaseByteArrayElements(env, in, Buf, 0);
//  (*env)->ReleaseByteArrayElements(env, out, Pixel, 0);
//
//  return consumed_bytes;
//
//}

// 接口函数-------------------------------------------------------------------------
extern "C" {

jlong Java_com_jxlianlian_masdk_core_Decoder_DecoderInit(JNIEnv *env, jobject ob, jint payloadType, jboolean isRGB, jobject nativeWindow) {
  long decoderHandle = 0;
  LOGI("Java_com_jxlianlian_masdk_VmPlayer_DecoderInit(%zd)", payloadType);
  ANativeWindow* pNativeWindow = ANativeWindow_fromSurface(env, nativeWindow);
  LOGI("format=(%d) width=(%d) height=(%d)", ANativeWindow_getFormat(pNativeWindow), ANativeWindow_getWidth(pNativeWindow), ANativeWindow_getHeight(pNativeWindow));
  bool ret = VmPlayer_DecoderInit(payloadType, isRGB, decoderHandle, (void*)pNativeWindow);
  if (!ret) {
    decoderHandle = 0;
  }
  return decoderHandle;
}

void Java_com_jxlianlian_masdk_core_Decoder_DecoderUninit(JNIEnv *env, jobject ob, jlong decoderHandle) {
  LOGI("Java_com_jxlianlian_masdk_VmPlayer_DecoderUninit(%zd)", decoderHandle);
  VmPlayer_DecoderUninit(decoderHandle);
}

jint Java_com_jxlianlian_masdk_core_Decoder_DecodeNalu(JNIEnv *env, jobject ob, jlong decoderHandle,
                                                       jbyteArray inData, jint inLen, jbyteArray outData) {
  //LOGI("Java_com_jxlianlian_masdk_VmPlayer_DecodeNalu(%d)", payloadType);

  jbyte *inBuf = env->GetByteArrayElements(inData, 0);
  jbyte *outBuf= env->GetByteArrayElements(outData, 0);

  int outlen = -1;
  bool ret = VmPlayer_DecodeNalu(decoderHandle, (const char*) inBuf, inLen, (char*) outBuf, outlen);
  if (!ret) {
    outlen = -1;
  }

  env->ReleaseByteArrayElements(inData, inBuf, 0);
  env->ReleaseByteArrayElements(outData, outBuf, 0);
  return outlen;
}

jint Java_com_jxlianlian_masdk_core_Decoder_GetFrameWidth(JNIEnv *env, jobject ob, jlong decoderHandle) {
  LOGI("Java_com_jxlianlian_masdk_core_Decoder_GetFrameWidth(%zd)", decoderHandle);
  return VmPlayer_GetFrameWidth(decoderHandle);
}

jint Java_com_jxlianlian_masdk_core_Decoder_GetFrameHeight(JNIEnv *env, jobject ob, jlong decoderHandle) {
  LOGI("Java_com_jxlianlian_masdk_core_Decoder_GetFrameHeight(%zd)", decoderHandle);
  return VmPlayer_GetFrameHeight(decoderHandle);
}

}