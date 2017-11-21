//
// Created by zhou rui on 2017/8/3.
//

#ifndef DREAM_DIVIDER_TYPE_H
#define DREAM_DIVIDER_TYPE_H

#include <stddef.h>
#include <stdint.h>
#include "../cipher/jw_cipher.h"

#define DREAM_DIVIDER_BUF_SIZE_MAX 818600

typedef struct {
    unsigned char buf[DREAM_DIVIDER_BUF_SIZE_MAX];
    size_t len;
    uint32_t separator;
    JW_CIPHER_CTX jw_cipher_ctx;
} DIVIDER_CTX;

#endif //VMSDKDEMO_ANDROID_COMMON_DIVIDER_H
