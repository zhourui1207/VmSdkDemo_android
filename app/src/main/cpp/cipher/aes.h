/*
 * aes.h
 *
 *  Created on: 2017年7月17日
 *      Author: zhourui
 */

#ifndef JW_AES_H_
#define JW_AES_H_

#include <stdint.h>

typedef struct {
	/*
	 * Number of columns (32-bit words) comprising the State. For this
	 * standard, Nb = 4.
	 */
	int Nb;

	/*
	 * Number of 32-bit words comprising the Cipher Key. For this
	 * standard, Nk = 4, 6, or 8.
	 */
	int Nk;

	/*
	 * Number of rounds, which is a function of  Nk  and  Nb (which is
	 * fixed). For this standard, Nr = 10, 12, or 14.
	 */
	int Nr;

	uint8_t key[16];

//	uint8_t expanded_key[176]; // uint8_t[Nb*(Nr+1)*4];

} AES_CTX;

void key_expansion(uint8_t *key, uint8_t *w, int Nb, int Nk, int Nr);

void cipher(AES_CTX* cipher, uint8_t *in, uint8_t *out, uint8_t *w);

void inv_cipher(AES_CTX* cipher, uint8_t *in, uint8_t *out, uint8_t *w);

#endif /* AES_H_ */
