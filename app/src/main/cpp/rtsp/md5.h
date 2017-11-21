#ifndef __md5_h__
#define __md5_h__

#include <cstddef>

typedef struct {
  unsigned int state[4];
  unsigned int count[2];
  unsigned char buffer[64];
} MD5_CTX;

typedef unsigned char *POINTER;
typedef unsigned short UINT2;
typedef unsigned int UINT4;

extern void MD5Init (MD5_CTX* context);
extern void MD5Update (MD5_CTX* context, const unsigned char* input, size_t inputLen);
extern void MD5Final (unsigned char digest[16], MD5_CTX* context);

#endif
