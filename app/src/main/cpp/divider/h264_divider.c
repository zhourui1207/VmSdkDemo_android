//
// Created by zhou rui on 2017/8/3.
//

#include <malloc.h>
#include "h264_divider.h"
#include "divider_type.h"
#include "../util/public/platform.h"

JW_DIVIDER_CTX divider_create() {
    DIVIDER_CTX *ctx = (DIVIDER_CTX *) malloc(sizeof(DIVIDER_CTX));
    ctx->len = 0;
    ctx->separator = UINT32_MAX;
    ctx->jw_cipher_ctx = NULL;
    return ctx;
}

void divider_release(JW_DIVIDER_CTX divider_ctx) {
    DIVIDER_CTX *ctx = (DIVIDER_CTX *) divider_ctx;
    if (ctx != NULL) {
        if (ctx->jw_cipher_ctx != NULL) {
            jw_cipher_release(ctx->jw_cipher_ctx);
            ctx->jw_cipher_ctx = NULL;
        }
        free((DIVIDER_CTX *) divider_ctx);
    }
}

void set_encrypt_key(JW_DIVIDER_CTX divider_ctx, const unsigned char *encrypt_key, size_t len) {
    DIVIDER_CTX *ctx = (DIVIDER_CTX *) divider_ctx;
    if (ctx != NULL && encrypt_key != NULL && len > 0) {
        if (ctx->jw_cipher_ctx != NULL) {
            jw_cipher_release(ctx->jw_cipher_ctx);
            ctx->jw_cipher_ctx = NULL;
        }
        ctx->jw_cipher_ctx = jw_cipher_create(encrypt_key, len);
    }
}

int input_buffer(JW_DIVIDER_CTX divider_ctx, const unsigned char *in, size_t len) {
    if (divider_ctx == NULL || in == NULL || len == 0) {
        return -1;
    }

    DIVIDER_CTX *ctx = (DIVIDER_CTX *) divider_ctx;
    if (ctx->separator == 1) {
        return 0;
    }

    int read_len = 0;
    for (read_len = 0; read_len < len; ++read_len) {
        unsigned char tmp = in[read_len];
        if (ctx->len == DREAM_DIVIDER_BUF_SIZE_MAX) {
            LOGE("h264_divider", "input length over max size!\n");
            ctx->len = 0;
        }
        ctx->buf[ctx->len++] = tmp;
        ctx->separator <<= 8;
        ctx->separator |= tmp;
        if (ctx->separator == 1 && ctx->len > 4) {  // h264或h265的分隔符
            ++read_len;
            break;
        }
    }

    return read_len;
}

int output_buffer(JW_DIVIDER_CTX divider_ctx, unsigned char *out, size_t len) {
    if (divider_ctx == NULL || out == NULL || len == 0) {
        return -1;
    }

    DIVIDER_CTX *ctx = (DIVIDER_CTX *) divider_ctx;
    int write_len = ctx->len - 4;
    if (write_len > 0 && (len < write_len)) {
        return -1;
    }

    if (ctx->separator != 1) {
        return 0;
    }

    ctx->separator = UINT32_MAX;

    // 解密
    if (ctx->jw_cipher_ctx != NULL) {
        jw_cipher_decrypt_h264(ctx->jw_cipher_ctx, ctx->buf, (size_t) write_len);
    }

    memcpy(out, ctx->buf, (size_t) write_len);
    return write_len;
}