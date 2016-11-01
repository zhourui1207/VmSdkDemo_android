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
#include <string.h>
#include <jni.h>
#include <stdio.h>

#include "libavutil/mem.h"
#include "libavcodec/avcodec.h"

#define TAG "VmPlayer-jni" // 这个是自定义的LOG的标识
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__) // 定义LOGD类型
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__) // 定义LOGI类型
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__) // 定义LOGW类型
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__) // 定义LOGE类型
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL, TAG, __VA_ARGS__) // 定义LOGF类型

#pragma comment(lib,"ijkffmpeg.lib")

#define CODEC_MAX_NUM 16

struct AVCodec* codec[CODEC_MAX_NUM] = {NULL};			  // Codec
struct AVCodecContext* c[CODEC_MAX_NUM] = {NULL};		  // Codec Context
struct AVFrame* picture[CODEC_MAX_NUM] = {NULL};		  // Frame

int iWidth[CODEC_MAX_NUM] = {0};
int iHeight[CODEC_MAX_NUM] = {0};

int* colortab[CODEC_MAX_NUM] = {NULL};
int* u_b_tab[CODEC_MAX_NUM] = {NULL};
int* u_g_tab[CODEC_MAX_NUM] = {NULL};
int* v_g_tab[CODEC_MAX_NUM] = {NULL};
int* v_r_tab[CODEC_MAX_NUM] = {NULL};

//short *tmp_pic=NULL;

unsigned int* rgb_2_pix[CODEC_MAX_NUM] = {NULL};
unsigned int* r_2_pix[CODEC_MAX_NUM] = {NULL};
unsigned int* g_2_pix[CODEC_MAX_NUM] = {NULL};
unsigned int* b_2_pix[CODEC_MAX_NUM] = {NULL};


void DeleteYUVTab(int index)
{
  av_free(colortab[index]);
  av_free(rgb_2_pix[index]);
}

void CreateYUVTab_16(int index)
{
  int i;
  int u, v;

  colortab[index] = (int *)av_malloc(4*256*sizeof(int));
  u_b_tab[index] = &(colortab[index][0*256]);
  u_g_tab[index] = &(colortab[index][1*256]);
  v_g_tab[index] = &(colortab[index][2*256]);
  v_r_tab[index] = &(colortab[index][3*256]);

  for (i=0; i<256; i++)
  {
    u = v = (i-128);

    u_b_tab[index][i] = (int) ( 1.772 * u);
    u_g_tab[index][i] = (int) ( 0.34414 * u);
    v_g_tab[index][i] = (int) ( 0.71414 * v);
    v_r_tab[index][i] = (int) ( 1.402 * v);
  }

  rgb_2_pix[index] = (unsigned int *)av_malloc(3*768*sizeof(unsigned int));

  r_2_pix[index] = &(rgb_2_pix[index][0*768]);
  g_2_pix[index] = &(rgb_2_pix[index][1*768]);
  b_2_pix[index] = &(rgb_2_pix[index][2*768]);

  for(i=0; i<256; i++)
  {
    r_2_pix[index][i] = 0;
    g_2_pix[index][i] = 0;
    b_2_pix[index][i] = 0;
  }

  for(i=0; i<256; i++)
  {
    r_2_pix[index][i+256] = (i & 0xF8) << 8;
    g_2_pix[index][i+256] = (i & 0xFC) << 3;
    b_2_pix[index][i+256] = (i ) >> 3;
  }

  for(i=0; i<256; i++)
  {
    r_2_pix[index][i+512] = 0xF8 << 8;
    g_2_pix[index][i+512] = 0xFC << 3;
    b_2_pix[index][i+512] = 0x1F;
  }

  r_2_pix[index] += 256;
  g_2_pix[index] += 256;
  b_2_pix[index] += 256;
}

void DisplayYUV_16(int index, unsigned int *pdst1, unsigned char *y, unsigned char *u, unsigned char *v, int width, int height, int src_ystride, int src_uvstride, int dst_ystride)
{
  int i, j;
  int r, g, b, rgb;

  int yy, ub, ug, vg, vr;

  unsigned char* yoff;
  unsigned char* uoff;
  unsigned char* voff;

  unsigned int* pdst=pdst1;

  int width2 = width/2;
  int height2 = height/2;

  if(width2>iWidth[index]/2)
  {
    width2=iWidth[index]/2;

    y+=(width-iWidth[index])/4*2;
    u+=(width-iWidth[index])/4;
    v+=(width-iWidth[index])/4;
  }

  if(height2>iHeight[index])
    height2=iHeight[index];

  for(j=0; j<height2; ++j)
  {
    yoff = y + j * 2 * src_ystride;
    uoff = u + j * src_uvstride;
    voff = v + j * src_uvstride;

    for(i=0; i<width2; ++i)
    {
      yy  = *(yoff+(i<<1));
      ub = u_b_tab[index][*(uoff+i)];
      ug = u_g_tab[index][*(uoff+i)];
      vg = v_g_tab[index][*(voff+i)];
      vr = v_r_tab[index][*(voff+i)];

      b = yy + ub;
      g = yy - ug - vg;
      r = yy + vr;

      rgb = r_2_pix[index][r] + g_2_pix[index][g] + b_2_pix[index][b];

      yy = *(yoff+(i<<1)+1);
      b = yy + ub;
      g = yy - ug - vg;
      r = yy + vr;

      pdst[(j*dst_ystride+i)] = (rgb)+((r_2_pix[index][r] + g_2_pix[index][g] + b_2_pix[index][b])<<16);

      yy = *(yoff+(i<<1)+src_ystride);
      b = yy + ub;
      g = yy - ug - vg;
      r = yy + vr;

      rgb = r_2_pix[index][r] + g_2_pix[index][g] + b_2_pix[index][b];

      yy = *(yoff+(i<<1)+src_ystride+1);
      b = yy + ub;
      g = yy - ug - vg;
      r = yy + vr;

      pdst [((2*j+1)*dst_ystride+i*2)>>1] = (rgb)+((r_2_pix[index][r] + g_2_pix[index][g] + b_2_pix[index][b])<<16);
    }
  }
}

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
//extern "C" {
jboolean Java_com_jxlianlian_masdk_VmPlayer_H264DecoderInit(JNIEnv *env, jobject ob) {
  LOGE("H264DecoderInit!!!%d", avcodec_version());

  c[0] = avcodec_alloc_context3(NULL);
  if (c[0] == NULL) {
    LOGE("c[0] = NULL");
  }
  LOGE("!!");
//
//  avcodec_open(c[index]);
//
//  picture[index] = avcodec_alloc_frame();//picture= malloc(sizeof(AVFrame));
//
//  return 1;
  return 1;
}

//}