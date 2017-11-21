/*
 * jw_cipher.c
 *
 *  Created on: 2017年7月17日
 *      Author: zhourui
 */

//#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include "aes.h"
#include "jw_aes.h"
#include "md5.h"
#include "jw_cipher.h"
#include "base64encode.h"
//#include <time.h>

static uint8_t jw_cipher_h264_box[256] = {
		// 0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f
		0xc4, 0xf7, 0x2e, 0x37, 0xe7, 0x24, 0xdc, 0x0c, 0xb8, 0xfa, 0xec, 0xbf, 0xb0, 0x0d, 0x22, 0x5a, // 0
		0x28, 0x0b, 0x99, 0x75, 0x4c, 0xd9, 0xf9, 0x86, 0x77, 0xcc, 0xa0, 0x1d, 0x79, 0x05, 0x27, 0x4e, // 1
		0x04, 0xef, 0xc0, 0x47, 0x5c, 0xf4, 0x2f, 0x1f, 0x11, 0x59, 0x67, 0xc2, 0x20, 0x3a, 0xe1, 0x2d, // 2
		0x1a, 0xeb, 0xfc, 0xb3, 0xb1, 0xa9, 0x7b, 0x12, 0x94, 0x51, 0x9b, 0x18, 0x76, 0x92, 0x80, 0x6a, // 3
		0xaf, 0xe4, 0xc6, 0x3c, 0x56, 0x6c, 0xf6, 0xb5, 0x8c, 0x9c, 0x44, 0x13, 0xdb, 0xb7, 0xe9, 0x0e, // 4
		0xed, 0xa6, 0x9d, 0x78, 0x31, 0xf5, 0x71, 0x82, 0x5e, 0x91, 0x15, 0x26, 0x65, 0xe0, 0x16, 0xc5, // 5
		0x40, 0x30, 0xad, 0x6f, 0x7c, 0x60, 0x3d, 0x9f, 0xba, 0x41, 0x73, 0x45, 0x29, 0x4b, 0x7a, 0x2b, // 6
		0x9a, 0xb6, 0x2a, 0x72, 0xd5, 0x85, 0x68, 0x83, 0x96, 0x88, 0x17, 0x46, 0xe8, 0x38, 0x61, 0xe2, // 7
		0x90, 0x62, 0xd2, 0xc3, 0x7e, 0xda, 0x6b, 0x49, 0x52, 0x39, 0xa2, 0xf3, 0xf0, 0x4a, 0xf2, 0x4f, // 8
		0x06, 0x8e, 0xbb, 0x58, 0x1b, 0xcb, 0x3e, 0xcd, 0x34, 0xfb, 0x70, 0xea, 0x25, 0x3b, 0xdf, 0xfd, // 9
		0x66, 0x95, 0xaa, 0xe6, 0x0f, 0x8b, 0xbe, 0xd3, 0xd6, 0x42, 0x57, 0xdd, 0xb4, 0x1e, 0xc8, 0x21, // a
		0x7f, 0x63, 0xe5, 0x6e, 0x74, 0xd7, 0x48, 0xd4, 0x23, 0x50, 0xa8, 0xc7, 0x10, 0x32, 0xce, 0xae, // b
		0x07, 0xa3, 0xe3, 0xca, 0x98, 0x1c, 0xd1, 0xde, 0x08, 0x64, 0x3f, 0x84, 0x69, 0xc9, 0x55, 0xbd, // c
		0xb2, 0x4d, 0x2c, 0x89, 0xac, 0xa4, 0x7d, 0x93, 0xf1, 0xd0, 0xcf, 0x8a, 0x53, 0x43, 0x33, 0xa1, // d
		0x09, 0xc1, 0x19, 0xab, 0x0a, 0x81, 0xbc, 0x9e, 0x87, 0x5b, 0x6d, 0xf8, 0xd8, 0xa5, 0x97, 0x5d, // e
		0x14, 0x54, 0x35, 0x8f, 0x00, 0xff, 0xa7, 0x36, 0x8d, 0xb9, 0x01, 0x03, 0x02, 0x5f, 0xee, 0xfe, // f
};

static uint8_t jw_cipher_box[256] = {
		// 0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f
		0xa4, 0xc3, 0xee, 0xfd, 0xca, 0x36, 0x22, 0x73, 0x14, 0xf1, 0xfb, 0x29, 0x70, 0xd1, 0x4e, 0xe7, // 0
		0xac, 0xa9, 0xf4, 0x9c, 0xc8, 0x92, 0x28, 0xaa, 0x52, 0x50, 0xe0, 0x45, 0xc1, 0xa3, 0x41, 0xeb, // 1
		0x04, 0xe9, 0x0f, 0xdc, 0xd2, 0x6b, 0x30, 0xa2, 0x69, 0x16, 0x49, 0xb0, 0x05, 0xfa, 0x5a, 0xec, // 2
		0x89, 0x19, 0x74, 0x8f, 0x71, 0x17, 0x8a, 0xf2, 0x67, 0x26, 0x5e, 0x5c, 0x75, 0xff, 0x6a, 0x33, // 3
		0xd6, 0xf0, 0x1f, 0x0d, 0x7e, 0x39, 0x1c, 0x12, 0x1e, 0x44, 0xa1, 0x76, 0xe3, 0x64, 0x4a, 0x0e, // 4
		0x4f, 0x9f, 0xc2, 0x8e, 0xd8, 0xb3, 0x27, 0x62, 0xb2, 0x7b, 0x31, 0x79, 0xf8, 0x97, 0x3d, 0xf9, // 5
		0x06, 0xb4, 0x77, 0xdb, 0xbc, 0xe6, 0x7f, 0x98, 0xa7, 0x2f, 0xde, 0xab, 0x94, 0xe8, 0x25, 0xe4, // 6
		0x23, 0xdd, 0x87, 0xa0, 0x07, 0xbb, 0x2a, 0xcc, 0xa8, 0xd7, 0xe5, 0x91, 0x0c, 0xb9, 0x78, 0x6f, // 7
		0x46, 0xd0, 0x8c, 0x42, 0x2c, 0xf6, 0xd5, 0x93, 0x57, 0xe1, 0x8b, 0x4d, 0x9d, 0xbe, 0x81, 0x7d, // 8
		0x08, 0x1d, 0xa6, 0x59, 0xce, 0x86, 0x09, 0xb1, 0x1b, 0xe2, 0xda, 0x1a, 0xc9, 0xb7, 0xd9, 0xc7, // 9
		0xa5, 0x3c, 0xfc, 0x37, 0x38, 0x43, 0xc4, 0x80, 0x84, 0x72, 0xae, 0x40, 0x56, 0x54, 0xf3, 0x65, // a
		0x0a, 0xd3, 0x68, 0xf5, 0x48, 0xba, 0x2d, 0x47, 0x3e, 0x13, 0x0b, 0x4c, 0x5b, 0xcb, 0x20, 0xfe, // b
		0x85, 0x2e, 0x55, 0xb8, 0xaf, 0x32, 0x53, 0x11, 0x10, 0x60, 0xad, 0x15, 0x3f, 0xc5, 0x9b, 0xc0, // c
		0x6c, 0xef, 0x6d, 0x96, 0x34, 0x6e, 0xbd, 0xdf, 0x18, 0xf7, 0x90, 0x5f, 0x83, 0x21, 0x9e, 0x3b, // d
		0x24, 0x99, 0x7a, 0x95, 0x2b, 0x63, 0x7c, 0x5d, 0xcf, 0xd4, 0x58, 0x88, 0x35, 0x4b, 0x02, 0x82, // e
		0x51, 0xed, 0x61, 0xea, 0xb6, 0xbf, 0x66, 0x03, 0x9a, 0xb5, 0x3a, 0xcd, 0x00, 0xc6, 0x01, 0x8d, // f
};

static uint8_t jw_cipher_cloud_pass_box[256] = {
		// 0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f
		0xa9, 0x9d, 0x91, 0x20, 0xe9, 0x90, 0xca, 0x44, 0xbf, 0xfb, 0xc2, 0xe7, 0x42, 0xb6, 0x2c, 0x92, // 0
		0xe6, 0x26, 0x5e, 0xa1, 0x0d, 0xc6, 0xfa, 0xe1, 0xc0, 0xe4, 0x9e, 0x58, 0x75, 0x71, 0xd0, 0x17, // 1
		0xe3, 0xd9, 0xa8, 0x16, 0xe2, 0xed, 0x10, 0x46, 0x87, 0xf6, 0x93, 0xa0, 0x53, 0xf8, 0xdd, 0x7c, // 2
		0x54, 0xee, 0xdb, 0xe5, 0xba, 0x47, 0x64, 0x1d, 0x73, 0x9f, 0xd8, 0x72, 0xd7, 0x86, 0x25, 0x4d, // 3
		0xd6, 0xaa, 0xbb, 0x31, 0x33, 0x19, 0x4e, 0x63, 0x09, 0x04, 0x3d, 0xc8, 0xd5, 0xb8, 0xf5, 0x11, // 4
		0xd3, 0x3f, 0x56, 0x59, 0xab, 0xf9, 0xeb, 0x79, 0xcf, 0x4a, 0x61, 0xce, 0x96, 0xe8, 0x12, 0xb4, // 5
		0xcc, 0x6b, 0xf3, 0xfe, 0x6c, 0x88, 0xc7, 0x05, 0x30, 0xc4, 0x94, 0xa6, 0x77, 0xc5, 0xd4, 0x2f, // 6
		0xc1, 0x21, 0xfc, 0x8a, 0xbd, 0xc3, 0x57, 0xa4, 0x8d, 0x6a, 0x28, 0x36, 0xb9, 0x9b, 0x4c, 0xef, // 7
		0x41, 0x1c, 0xb5, 0x7e, 0x69, 0x70, 0x76, 0x49, 0xb3, 0xa7, 0x14, 0xf2, 0x6f, 0x38, 0xf1, 0x89, // 8
		0xb0, 0x48, 0xaf, 0x1a, 0x15, 0x2a, 0x97, 0xae, 0x98, 0xad, 0x6e, 0xcb, 0xf7, 0xa3, 0xa2, 0x3b, // 9
		0x9c, 0x68, 0x55, 0x83, 0x0c, 0x65, 0x9a, 0xe0, 0x95, 0x8b, 0x4b, 0xcd, 0x8e, 0xb7, 0x2e, 0x1b, // a
		0x85, 0xdf, 0xec, 0x81, 0x4f, 0x32, 0x03, 0xa5, 0x7f, 0xf4, 0x7b, 0xb1, 0x34, 0x3c, 0x78, 0x84, // b
		0x6d, 0x67, 0xff, 0x07, 0x62, 0x22, 0x06, 0xdc, 0x5f, 0x5d, 0x24, 0x3a, 0x40, 0x0a, 0xea, 0x5b, // c
		0x5a, 0xd1, 0x1e, 0x51, 0x50, 0xbc, 0x43, 0x66, 0x39, 0xd2, 0xc9, 0x08, 0x2d, 0x2b, 0x7d, 0x27, // d
		0x23, 0xfd, 0x18, 0x0b, 0x13, 0x60, 0xb2, 0xda, 0x01, 0x45, 0xf0, 0xbe, 0x99, 0x82, 0x80, 0x1f, // e
		0x5c, 0x3e, 0x35, 0x7a, 0x02, 0xde, 0x8c, 0x37, 0x0e, 0x8f, 0x0f, 0x52, 0x29, 0x74, 0xac, 0x00, // f
};

static uint8_t jw_cipher_cloud_pass_box_ext[256] = {
		// 0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f
		0x79, 0xcf, 0x31, 0x98, 0x05, 0x89, 0x3f, 0x50, 0xf1, 0x66, 0xff, 0x1b, 0x27, 0x9f, 0x08, 0xfe, // 0
		0x2d, 0x48, 0x15, 0x73, 0x45, 0x87, 0x43, 0x9a, 0xb5, 0xd8, 0x5d, 0xa7, 0x2f, 0x8e, 0x61, 0x65, // 1
		0x6a, 0x52, 0x1a, 0xdd, 0xbf, 0xfd, 0x01, 0x39, 0x33, 0x9c, 0x2c, 0xd5, 0x8d, 0x6b, 0xa6, 0x94, // 2
		0xaf, 0x83, 0xd0, 0x68, 0x44, 0x85, 0x67, 0x51, 0xd2, 0x9b, 0x7e, 0x8b, 0xe5, 0xe9, 0x17, 0x26, // 3
		0xf6, 0x96, 0x37, 0x5c, 0xad, 0xec, 0x40, 0xed, 0x74, 0x8f, 0x2a, 0xa1, 0x3a, 0xd1, 0x54, 0x03, // 4
		0x6f, 0x5a, 0xee, 0x97, 0xfa, 0x3c, 0xbc, 0x1c, 0x8c, 0x62, 0x7b, 0xf8, 0x81, 0x02, 0xe8, 0x11, // 5
		0xc5, 0xbb, 0x24, 0xce, 0xe0, 0x18, 0xc4, 0xb3, 0x47, 0x8a, 0x10, 0x23, 0xb0, 0x2e, 0xfc, 0x1d, // 6
		0x4d, 0x0c, 0xc2, 0x90, 0x75, 0x6c, 0x46, 0xd6, 0xe3, 0x53, 0xf4, 0xdf, 0x38, 0x5b, 0xda, 0xab, // 7
		0xf0, 0x64, 0x0a, 0x80, 0xde, 0xfb, 0x4e, 0x0e, 0x59, 0x28, 0x91, 0x86, 0x6d, 0xb9, 0x5f, 0xc0, // 8
		0xca, 0x34, 0x7c, 0x04, 0x69, 0x58, 0x12, 0x41, 0x56, 0xa9, 0x99, 0xcb, 0xaa, 0xf3, 0xc7, 0x30, // 9
		0xea, 0x77, 0x4a, 0x92, 0xe1, 0x60, 0x7d, 0xb1, 0xd4, 0xd9, 0xf5, 0x71, 0x0d, 0xe7, 0xf9, 0xcc, // a
		0x84, 0x49, 0xbd, 0x88, 0x95, 0x1f, 0x42, 0xf7, 0xa0, 0x78, 0xd7, 0x7f, 0xc1, 0xe4, 0x63, 0xb4, // b
		0xdb, 0x0f, 0xb8, 0xa2, 0xac, 0x07, 0xbe, 0xb2, 0x93, 0xe6, 0xb7, 0x19, 0x9e, 0x09, 0x29, 0x72, // c
		0xcd, 0x7a, 0xba, 0xe2, 0xa3, 0xeb, 0x16, 0xf2, 0x06, 0x57, 0x14, 0x82, 0x21, 0xae, 0x6e, 0xc9, // d
		0xb6, 0x4f, 0x3b, 0x5e, 0x22, 0x55, 0x32, 0x4b, 0xa8, 0x70, 0x35, 0x25, 0x0b, 0x20, 0x3d, 0x1e, // e
		0xa5, 0xc8, 0x4c, 0xa4, 0x76, 0xc6, 0x13, 0xc3, 0x2b, 0xdc, 0xef, 0x36, 0x3e, 0xd3, 0x9d, 0x00, // f
};

static const int bits = 128;

void domd5(const unsigned char* input, size_t inputlen, unsigned char* digest)
{
	MD5_CTX ctx;
	MD5Init(&ctx);
	MD5Update(&ctx, input, inputlen);
	MD5Final(digest, &ctx);
}

void generate_key(uint8_t key[16], const unsigned char* key_str, size_t key_len) {
	// md5
	domd5(key_str, key_len, key);
}

void change_key(const uint8_t src_key[16], uint8_t dst_key[16]) {
	// 转换
	int row, col;
	uint8_t tmp[16];
	int i = 0;
	memcpy(tmp, src_key, 16);
	for (; i < 16; ++i) {
		row = (tmp[i] & 0xf0) >> 4;
		col = tmp[i] & 0x0f;
		tmp[i] = jw_cipher_box[16*row+col];
	}

	domd5(tmp, 16, dst_key);
}

void change_h264_key(const uint8_t src_key[16], uint8_t dst_key[16]) {
	// 转换
	int row, col;
	uint8_t tmp[16];
	int i = 0;
	memcpy(tmp, src_key, 16);
	for (; i < 16; ++i) {
		row = (tmp[i] & 0xf0) >> 4;
		col = tmp[i] & 0x0f;
		tmp[i] = jw_cipher_h264_box[16*row+col];
	}

	domd5(tmp, 16, dst_key);
}

//void get_expanded_key(const uint8_t src_key[16], uint8_t expanded_key[176], int Nb, int Nk, int Nr) {
//	uint8_t key[16];
//	change_key(src_key, key);
//
//	key_expansion(key, expanded_key, Nb, Nk, Nr);
//}

//void get_h264_expanded_key(const uint8_t src_key[16], uint8_t expanded_key[176], int Nb, int Nk, int Nr) {
//	uint8_t key[16];
//	change_h264_key(src_key, key);
//	key_expansion(key, expanded_key, Nb, Nk, Nr);
//}

//API_DECL JW_CIPHER_CTX jw_cipher_create(const unsigned char* key_str, size_t key_len) {
////	printf("key:%s, key len:%d\n", key, key_len);
//	AES_CTX* aes_ctx = NULL;
//
//	if (NULL == key_str || key_len == 0) {
//		printf("Key can't be NULL or 0 length! \n");
//		return aes_ctx;
//	}
//
//	aes_ctx = (AES_CTX*) malloc(sizeof(AES_CTX));
//	aes_ctx->Nb = 4;
//	aes_ctx->Nk = 4;
//	aes_ctx->Nr = 10;
//	// 计算出key
//	generate_key(aes_ctx->key, key_str, key_len);
//	return aes_ctx;
//}

API_DECL JW_CIPHER_CTX jw_cipher_create(const unsigned char* key_str, size_t key_len) {
	JW_AES_USER_KEY* aes_ctx = NULL;

	if (NULL == key_str || key_len == 0) {
		printf("Key can't be NULL or 0 length! \n");
		return aes_ctx;
	}

	aes_ctx = (JW_AES_USER_KEY*) malloc(sizeof(JW_AES_USER_KEY));

	// 计算出key
	generate_key(aes_ctx->key, key_str, key_len);
	return aes_ctx;
}

//API_DECL void jw_cipher_release(JW_CIPHER_CTX ctx) {
//	if (NULL != ctx) {
//		AES_CTX* aes_ctx = (AES_CTX*) ctx;
//		free(aes_ctx);
//	}
//}

API_DECL void jw_cipher_release(JW_CIPHER_CTX ctx) {
	if (NULL != ctx) {
		JW_AES_USER_KEY* aes_ctx = (JW_AES_USER_KEY*) ctx;
		free(aes_ctx);
	}
}

#if (defined _ONLY_ENCRYPT) || (defined _FULL_FUNCTIONAL)
//API_DECL void jw_cipher_encrypt_h264(JW_CIPHER_CTX ctx, unsigned char* input_data, size_t input_data_len) {
//	if (NULL != ctx && NULL != input_data && input_data_len > 4) {
//		AES_CTX* aes_ctx = (AES_CTX*) ctx;
//		int pos = 0;
//		// 有起始字节
//		if (input_data[0] == 0 && input_data[1] == 0 && input_data[2] == 0 && input_data[3] == 1 && ((input_data[4] & 0x1F) == 5) && input_data_len >= 21) {
//			pos = 5;
//		} else if (((input_data[0] & 0x1F) == 5) && input_data_len >= 17) {  // 无起始字节
//			pos = 1;
//		} else if ((input_data[0] == 0 && input_data[1] == 0 && input_data[2] == 0 && input_data[3] == 1 && ((input_data[4] & 0x1F) == 7)) || ((input_data[0] & 0x1F) == 7)) {  // sps
//			unsigned char tmp[5];  // 判断5个字节
//			int i = 0;
//			memset(tmp, 0xff, 5);
//			for (; i < input_data_len; ++i) {
//				tmp[0] = tmp[1];
//				tmp[1] = tmp[2];
//				tmp[2] = tmp[3];
//				tmp[3] = tmp[4];
//				tmp[4] = input_data[i];
//				if (tmp[0] == 0 && tmp[1] == 0 && tmp[2] == 0 && tmp[3] == 1 && ((tmp[4] & 0x1F) == 5)) {
//					if (i + 16 < input_data_len) {
//						pos = i + 1;
//					}
//					break;
//				}
//			}
//
//		}
//		if (pos > 0) {
//			uint8_t expanded_key[176];
//			uint8_t tmp[16];
//			get_h264_expanded_key(aes_ctx->key, expanded_key, aes_ctx->Nb, aes_ctx->Nk, aes_ctx->Nr);
//			cipher(ctx, (uint8_t*) (input_data + pos), tmp,  expanded_key);
//			memcpy(input_data + pos, tmp, 16);
//		}
//	}
//}

API_DECL void jw_cipher_encrypt_h264(JW_CIPHER_CTX ctx, unsigned char* input_data, size_t input_data_len) {
	if (NULL != ctx && NULL != input_data && input_data_len > 4) {
		JW_AES_USER_KEY* aes_ctx = (JW_AES_USER_KEY*) ctx;
		int pos = 0;
		// 有起始字节
		if (input_data[0] == 0 && input_data[1] == 0 && input_data[2] == 0 && input_data[3] == 1 && ((input_data[4] & 0x1F) == 5) && input_data_len >= 21) {
			pos = 5;
		} else if (((input_data[0] & 0x1F) == 5) && input_data_len >= 17) {  // 无起始字节
			pos = 1;
		} else if ((input_data[0] == 0 && input_data[1] == 0 && input_data[2] == 0 && input_data[3] == 1 && ((input_data[4] & 0x1F) == 7)) || ((input_data[0] & 0x1F) == 7)) {  // sps
			unsigned char tmp[5];  // 判断5个字节
			int i = 0;
			memset(tmp, 0xff, 5);
			for (; i < input_data_len; ++i) {
				tmp[0] = tmp[1];
				tmp[1] = tmp[2];
				tmp[2] = tmp[3];
				tmp[3] = tmp[4];
				tmp[4] = input_data[i];
				if (tmp[0] == 0 && tmp[1] == 0 && tmp[2] == 0 && tmp[3] == 1 && ((tmp[4] & 0x1F) == 5)) {
					if (i + 16 < input_data_len) {
						pos = i + 1;
					}
					break;
				}
			}

		}
		if (pos > 0) {
			uint8_t dst_key[16];
			change_h264_key(aes_ctx->key, dst_key);
			JW_AES_KEY aes_key;
			if (JW_AES_set_encrypt_key(dst_key, bits, &aes_key) == 0) {
				uint8_t* i_frame_data = input_data + pos;
				JW_AES_encrypt(i_frame_data, i_frame_data, &aes_key);
			}
		}
	}
}
#endif

#if (defined _ONLY_DECRYPT) || (defined _FULL_FUNCTIONAL)
//API_DECL void jw_cipher_decrypt_h264(JW_CIPHER_CTX ctx, unsigned char* input_data, size_t input_data_len) {
//	if (NULL != ctx && NULL != input_data && input_data_len > 4) {
//		AES_CTX* aes_ctx = (AES_CTX*) ctx;
//		int pos = 0;
//		// 有起始字节
//		if (input_data[0] == 0 && input_data[1] == 0 && input_data[2] == 0 && input_data[3] == 1 && ((input_data[4] & 0x1F) == 5) && input_data_len >= 21) {
//			pos = 5;
//		} else if (((input_data[0] & 0x1F) == 5) && input_data_len >= 17) {  // 无起始字节
//			pos = 1;
//		} else if ((input_data[0] == 0 && input_data[1] == 0 && input_data[2] == 0 && input_data[3] == 1 && ((input_data[4] & 0x1F) == 7)) || ((input_data[0] & 0x1F) == 7)) {  // sps
//			unsigned char tmp[5];  // 判断5个字节
//			int i = 0;
//			memset(tmp, 0xff, 5);
//			for (; i < input_data_len; ++i) {
//				tmp[0] = tmp[1];
//				tmp[1] = tmp[2];
//				tmp[2] = tmp[3];
//				tmp[3] = tmp[4];
//				tmp[4] = input_data[i];
//				if (tmp[0] == 0 && tmp[1] == 0 && tmp[2] == 0 && tmp[3] == 1 && ((tmp[4] & 0x1F) == 5)) {
//					if (i + 16 < input_data_len) {
//						pos = i + 1;
//					}
//					break;
//				}
//			}
//
//		}
//		if (pos > 0) {
//			uint8_t expanded_key[176];
//			uint8_t tmp[16];
//			get_h264_expanded_key(aes_ctx->key, expanded_key, aes_ctx->Nb, aes_ctx->Nk, aes_ctx->Nr);
//			inv_cipher(ctx, (uint8_t*) (input_data + pos), tmp,  expanded_key);
//			memcpy(input_data + pos, tmp, 16);
//		}
//	}
//}

API_DECL void jw_cipher_decrypt_h264(JW_CIPHER_CTX ctx, unsigned char* input_data, size_t input_data_len) {
	if (NULL != ctx && NULL != input_data && input_data_len > 4) {
			JW_AES_USER_KEY* aes_ctx = (JW_AES_USER_KEY*) ctx;
			int pos = 0;
			// 有起始字节
			if (input_data[0] == 0 && input_data[1] == 0 && input_data[2] == 0 && input_data[3] == 1 && ((input_data[4] & 0x1F) == 5) && input_data_len >= 21) {
				pos = 5;
			} else if (((input_data[0] & 0x1F) == 5) && input_data_len >= 17) {  // 无起始字节
				pos = 1;
			} else if ((input_data[0] == 0 && input_data[1] == 0 && input_data[2] == 0 && input_data[3] == 1 && ((input_data[4] & 0x1F) == 7)) || ((input_data[0] & 0x1F) == 7)) {  // sps
				unsigned char tmp[5];  // 判断5个字节
				int i = 0;
				memset(tmp, 0xff, 5);
				for (; i < input_data_len; ++i) {
					tmp[0] = tmp[1];
					tmp[1] = tmp[2];
					tmp[2] = tmp[3];
					tmp[3] = tmp[4];
					tmp[4] = input_data[i];
					if (tmp[0] == 0 && tmp[1] == 0 && tmp[2] == 0 && tmp[3] == 1 && ((tmp[4] & 0x1F) == 5)) {
						if (i + 16 < input_data_len) {
							pos = i + 1;
						}
						break;
					}
				}

			}
			if (pos > 0) {
				uint8_t dst_key[16];
				change_h264_key(aes_ctx->key, dst_key);
				JW_AES_KEY aes_key;
				if (JW_AES_set_decrypt_key(dst_key, bits, &aes_key) == 0) {
					uint8_t* i_frame_data = input_data + pos;
					JW_AES_decrypt(i_frame_data, i_frame_data, &aes_key);
				}
			}
		}
}
#endif

API_DECL size_t jw_cipher_encrypt_output_len(size_t input_data_len) {
	return (input_data_len / 16) * 16 + 16;
}

//API_DECL int jw_cipher_encrypt(JW_CIPHER_CTX ctx, const unsigned char* input_data, size_t input_data_len, unsigned char* output_data, size_t* output_data_len) {
//	size_t out_len;
//	AES_CTX* aes_ctx;
//	uint8_t expanded_key[176];
//	uint8_t tmp[16];
//	int round;
//	int i = 0;
//	uint8_t fill;
//	uint8_t fill_input[16];
//
//	if (input_data == NULL || input_data_len == 0 || output_data == NULL || *output_data_len < 16) {
//		return 0;
//	}
//
//	// 加密后长度
//	out_len = (input_data_len / 16) * 16 + 16;
//	if (*output_data_len < out_len) {
//		return 0;
//	}
//	// 先进行补齐
//	aes_ctx = (AES_CTX*) ctx;
//	get_expanded_key(aes_ctx->key, expanded_key, aes_ctx->Nb, aes_ctx->Nk, aes_ctx->Nr);
//	round = input_data_len / 16;  // 轮数
//	for (; i < round; ++i) {
//		cipher(ctx, (uint8_t*) (input_data + i * 16), tmp,  expanded_key);
//		memcpy(output_data + i * 16, tmp, 16);
//	}
//
//	// 剩下的
//	fill = out_len - input_data_len;
//	memset(fill_input, fill, 16);
//	memcpy(fill_input, input_data + i * 16, input_data_len % 16);
//
//	cipher(ctx, fill_input, tmp,  expanded_key);
//	memcpy(output_data + i * 16, tmp, 16);
//
//	*output_data_len = out_len;
//
//	return 1;
//}

API_DECL int jw_cipher_encrypt(JW_CIPHER_CTX ctx, const unsigned char* input_data, size_t input_data_len, unsigned char* output_data, size_t* output_data_len) {
	size_t out_len;
	JW_AES_USER_KEY* aes_ctx;
	uint8_t dst_key[16];
	int round;
	int i = 0;
	uint8_t fill;
	uint8_t fill_input[16];

	if (input_data == NULL || input_data_len == 0 || output_data == NULL || *output_data_len < 16) {
		return 0;
	}

	// 加密后长度
	out_len = (input_data_len / 16) * 16 + 16;
	if (*output_data_len < out_len) {
		return 0;
	}
	// 先进行补齐
	aes_ctx = (JW_AES_USER_KEY*) ctx;
	change_key(aes_ctx->key, dst_key);

	JW_AES_KEY aes_key;
	if (JW_AES_set_encrypt_key(dst_key, bits, &aes_key) != 0) {
		return 0;
	}

	round = input_data_len / 16;  // 轮数
	for (; i < round; ++i) {
		JW_AES_encrypt(input_data + i * 16, output_data + i * 16, &aes_key);
	}

	// 剩下的
	fill = out_len - input_data_len;
	memset(fill_input, fill, 16);
	memcpy(fill_input, input_data + i * 16, input_data_len % 16);

	JW_AES_encrypt(fill_input, output_data + i * 16, &aes_key);

	*output_data_len = out_len;

	return 1;
}

//API_DECL int jw_cipher_decrypt(JW_CIPHER_CTX ctx, const unsigned char* input_data, size_t input_data_len, unsigned char* output_data, size_t* output_data_len) {
//	AES_CTX* aes_ctx;
//	uint8_t expanded_key[176];
//	uint8_t tmp[16];
//	int round;
//	int i = 0;
//	uint8_t fill;
//
//	if (input_data == NULL || input_data_len == 0 || output_data == NULL || *output_data_len == 0) {
//		return 0;
//	}
//
//	if ((input_data_len % 16 != 0) || (*output_data_len < input_data_len)) {  // 不能被16整除
//		return 0;
//	}
//
//	// 先进行补齐
//	aes_ctx = (AES_CTX*) ctx;
//	get_expanded_key(aes_ctx->key, expanded_key, aes_ctx->Nb, aes_ctx->Nk, aes_ctx->Nr);
//
//	round = input_data_len / 16;  // 轮数
//
//	for (; i < round; ++i) {
//		inv_cipher(ctx, (uint8_t*) (input_data + i * 16), tmp,  expanded_key);
//		memcpy(output_data + i * 16, tmp, 16);
//	}
//
//	// 判断最后一个字节
//	fill = output_data[input_data_len - 1];
//	*output_data_len = input_data_len - fill;
//
//	return 1;
//}

API_DECL int jw_cipher_decrypt(JW_CIPHER_CTX ctx, const unsigned char* input_data, size_t input_data_len, unsigned char* output_data, size_t* output_data_len) {
	JW_AES_USER_KEY* aes_ctx;
	uint8_t dst_key[16];
	int round;
	int i = 0;
	uint8_t fill;

	if (input_data == NULL || input_data_len == 0 || output_data == NULL || *output_data_len == 0) {
		return 0;
	}

	if ((input_data_len % 16 != 0) || (*output_data_len < input_data_len)) {  // 不能被16整除
		return 0;
	}

	// 先进行补齐
	aes_ctx = (JW_AES_USER_KEY*) ctx;
	change_key(aes_ctx->key, dst_key);

	JW_AES_KEY aes_key;
	if (JW_AES_set_decrypt_key(dst_key, bits, &aes_key) != 0) {
		return 0;
	}

	round = input_data_len / 16;  // 轮数
	for (; i < round; ++i) {
		JW_AES_decrypt(input_data + i * 16, output_data + i * 16, &aes_key);
	}

	// 判断最后一个字节
	fill = output_data[input_data_len - 1];
	*output_data_len = input_data_len - fill;

	return 1;
}

API_DECL int jw_cipher_cloud_pass(const char* input_pass, size_t input_pass_len, char* output_pass, size_t* output_pass_len) {
	// 转换
	int row, col;
	char extStr[11] = "joyware.com";
	char* tmpBuf;
	uint8_t sum = 0;
	int i = 0;
	uint8_t digest[16] = {0};
	char base64out[24] = {0};

	if (input_pass == NULL || input_pass_len == 0 || output_pass == NULL || (*output_pass_len < 22)) {
		return 0;
	}

	tmpBuf = (char*) malloc(input_pass_len + 12);

	for (; i < input_pass_len; ++i) {
		sum += (uint8_t)input_pass[i];
		row = (input_pass[i] & 0xf0) >> 4;
		col = input_pass[i] & 0x0f;
		tmpBuf[i] = jw_cipher_cloud_pass_box[16*row+col];
	}
	memcpy(tmpBuf + input_pass_len, extStr, 11);
	tmpBuf[input_pass_len + 11] = sum;

	domd5(tmpBuf, input_pass_len + 12, digest);
//	domd5(input_pass, input_pass_len, digest);

	free(tmpBuf);

	i = 0;
	for (; i < 16; ++i) {
		row = (digest[i] & 0xf0) >> 4;
		col = digest[i] & 0x0f;
		digest[i] = jw_cipher_cloud_pass_box_ext[16*row+col];
	}

	base64_encode(digest, 16, base64out);

	memcpy(output_pass, base64out, 22);
	*output_pass_len = 22;

//	memcpy(output_pass, digest, 16);
//	*output_pass_len = 16;

	return 1;
}

//int main(int argc, char *argv[]) {
//	int input_len = 3000;
//	uint8_t in[3000] = {0x00, 0x00, 0x00, 0x01, 0x65, 0xcc, 0x16, 0x21, 0x23, 0x11, 0xfe, 0xcc, 0x16, 0x21, 0x23, 0x11,
//			0x16, 0x21, 0x23, 0x11, 0xfe, 0xcc, 0x16, 0x21, 0x23, 0x11, 0xfe, 0xcc, 0x16, 0x21, 0x23, 0x11,
//			0x16, 0x21, 0x23, 0x11, 0xfe, 0xcc, 0x16, 0x21, 0x23, 0x11, 0xfe, 0xcc, 0x16, 0x21, 0x23, 0x11,
//			0x55};
//
//	time_t begin = time(0);
//
//	size_t out_len = input_len + 16;
//	uint8_t out[out_len];
//
//	int count = 1000000;
//	printf("count: %d\n", count);
//
//	size_t buf_len = out_len;
//	uint8_t buf[buf_len];
//
//	JW_CIPHER_CTX ctx = jw_cipher_create("abceef001267", 12);
//	for (int i = 0; i < count; ++i) {
//		jw_cipher_encrypt_h264(ctx, in, input_len);
////		jw_cipher_decrypt_h264_v2(ctx, in, input_len);
//	}
//
////	for (int i = 0; i < count; ++i) {
////		JW_AES_KEY aes_key;
////		if (JW_AES_set_encrypt_key("1234567890123456", 128, &aes_key) == 0) {
////			for (int j = 0; j < 188; ++j) {
////				JW_AES_encrypt(in, out, &aes_key);
////			}
////		}
//////
//////		AES_KEY aes_de_key;
//////		if (AES_set_decrypt_key("1234567890123456", 128, &aes_de_key) == 0) {
//////				AES_decrypt(out, buf, &aes_de_key);
//////		}
////	}
//
//
//	printf("time: %ld\n", time(0) - begin);
//	printf("-----------------\n");
//	for (int i = 0; i < input_len; ++i) {
//		printf("0x%02x ", in[i]);
//		if ((i + 1) % 4 == 0) {
//			printf("\n");
//		}
//	}
//
//	printf("-----------------\n");
//	for (int i = 0; i < out_len; ++i) {
//		printf("0x%02x ", out[i]);
//		if ((i + 1) % 4 == 0) {
//			printf("\n");
//		}
//	}
//
//	printf("-----------------\n");
//	for (int i = 0; i < buf_len; ++i) {
//		printf("0x%02x ", buf[i]);
//		if ((i + 1) % 4 == 0) {
//			printf("\n");
//		}
//	}
//
//
////	printf("\nencrypt data:---------------\n");
////	for (int i = 0; i < input_len; i++) {
////		printf("0x%02x ", in[i]);
////		if ((i + 1) % 4 == 0) {
////			printf("\n");
////		}
////	}
////
////	jw_cipher_decrypt_h264(ctx, (char*)in, input_len);
////
////	printf("\ndecrypt data:---------------\n");
////	for (int i = 0; i < input_len; i++) {
////		printf("0x%02x ", in[i]);
////		if ((i + 1) % 4 == 0) {
////			printf("\n");
////		}
////	}
////
////	printf("\nbegin test:---------------\n");
////	time_t t1 = time(NULL);
////
////	int testNumber = 100000;
////	for (int i = 0; i < testNumber; ++i) {
////		jw_cipher_encrypt_h264(ctx, (char*)in, input_len);
////	}
////
////	time_t t2 = time(NULL);
////	printf("\nend test:---------------time:[%d]\n", (t2 - t1));
////
//////	uint8_t indata[] = {
//////	//			0x00, 0x00, 0x00, 0x01,
//////				0x65, 0x55, 0x66, 0x77,
//////				0x88, 0x99, 0xaa, 0xbb,
//////				0xcc, 0xdd, 0xee, 0xff,
//////				0x12, 0x13, 0x00, 0x21,
//////				0x65, 0x55, 0x66, 0x77,
//////				0x88, 0x99, 0xaa, 0xbb,
//////				0xcc, 0xdd, 0xee, 0xff,
//////				0x12, 0x13, 0x00, 0x21,
//////				0x65, 0x55, 0x66, 0x77,
//////				0x88, 0x99, 0xaa, 0xbb,
//////				0xcc, 0xdd, 0xee, 0xff,
//////				0x12, 0x13, 0x00, 0x21,
//////				0x65, 0x55, 0x66, 0x77,
//////				0x88, 0x99, 0xaa, 0xbb,
//////				0xcc, 0xdd, 0xee, 0xff,
//////				0x12, 0x13, 0x00, 0x21,
//////				0x68
//////	};
////
////	char indata[100*1024] = "我了个去啊";
////
////	input_len = 100*1024;
////	size_t outlen = jw_cipher_encrypt_output_len(input_len);
////	char out_data[outlen];
////
////	printf("\nall begin test:---------------\n");
////	t1 = time(NULL);
////
////	int byteTestNumber = 10;
////	for (int i = 0; i < byteTestNumber; ++i) {
////		jw_cipher_encrypt(ctx, indata, input_len, out_data, &outlen);
////	}
////
////	t2 = time(NULL);
////	printf("\nall end test:---------------time:[%d]\n", (t2 - t1));
////
////	printf("\nencrypt data:---------------len[%d]\n", outlen);
//////	for (int i = 0; i < outlen; i++) {
//////		printf("0x%02x ", out_data[i]);
//////		if ((i + 1) % 4 == 0) {
//////			printf("\n");
//////		}
//////	}
////
////	unsigned char out_data2[outlen];
////	printf("\n de all begin test:---------------\n");
////	t1 = time(NULL);
////
////	for (int i = 0; i < byteTestNumber; ++i) {
////		jw_cipher_decrypt(ctx, out_data, outlen, out_data2, &outlen);
////	}
////
////	t2 = time(NULL);
////	printf("\n de all end test:---------------time:[%d]\n", (t2 - t1));
////
////
//////	printf("\decrypt data:---------------data[%s]\n", out_data2);
//////	for (int i = 0; i < outlen; i++) {
//////		printf("0x%02x ", out_data2[i]);
//////		if ((i + 1) % 4 == 0) {
//////			printf("\n");
//////		}
//////	}
////
////	jw_cipher_release(ctx);
//
////	JW_CIPHER_CTX ctx = jw_cipher_create("abceef001267", 12);
////
////	char indata[100] = "我了个去啊!";
////
////	size_t outlen = jw_cipher_encrypt_output_len(100);
////	char out_data[1000];
////	if (outlen <= 1000) {
////		if (jw_cipher_encrypt(ctx, indata, 100, out_data, &outlen) == 1) {
////			char out_data2[1000];
////			size_t outlen2 = 1000;
////			if (jw_cipher_decrypt(ctx, out_data, outlen, out_data2, &outlen2) == 1) {
////				printf("%s\n", out_data2);
////				return 0;
////			}
////		}
////	}
////	jw_cipher_release(ctx);
////	return -1;
//	//	for (int i = 0; i < 16; ++i) {
//	//		sprintf(out + 2*i, "%02x", digest[i]);
//	//		printf("%02x", digest[i]);
//	//	}
//	//	printf("\n");
////	JW_CIPHER_CTX ctx = jw_cipher_create("abceef001267", 12);
////
////		char indata_2[100] = "abcdef";
////
////		size_t outlen_2 = jw_cipher_encrypt_output_len(100);
////		char out_data_2[1000];
////		if (outlen_2 > 1000) {
////			return -1;
////		}
////		jw_cipher_encrypt(ctx, indata_2, strlen(indata_2), out_data_2, &outlen_2);
////
////		char out_data2[1000];
////		size_t outlen2 = 1000;
////		jw_cipher_decrypt(ctx, out_data_2, outlen_2, out_data2, &outlen2);
////		printf("%s\n", out_data2);
////
////		jw_cipher_release(ctx);
////
////		char out[34] = {0};
////		char indata[100] = {0};
////		char *user = "34020000001180000008";
////		char *pwd = "ABCDEF";
////		strcpy(indata, user);
////		strcat(indata, pwd);
////
////		size_t outlen = 34;
////		size_t inlen = strlen(indata);
////		printf("inlen=[%d]\n", inlen);
////		jw_cipher_cloud_pass(indata, inlen, out, &outlen);
////
////		printf("out[%s] outlen[%d]\n", out, outlen);
//		return 0;
//}


