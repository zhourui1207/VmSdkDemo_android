//
// Created by zhou rui on 2017/8/3.
//

#ifndef DREAM_H264_DIVIDER_H
#define DREAM_H264_DIVIDER_H

#include <stddef.h>

#define JW_DIVIDER_CTX void*

JW_DIVIDER_CTX divider_create();

void divider_release(JW_DIVIDER_CTX divider_ctx);

void set_encrypt_key(JW_DIVIDER_CTX divider_ctx, const unsigned char *encrypt_key, size_t len);

int input_buffer(JW_DIVIDER_CTX divider_ctx, const unsigned char *in, size_t len);

int output_buffer(JW_DIVIDER_CTX divider_ctx, unsigned char *out, size_t len);

#endif //VMSDKDEMO_ANDROID_H264DIVIDER_H
