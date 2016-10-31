/*
 * StringUtil.h
 *
 *  Created on: 2016年10月23日
 *      Author: zhourui
 */

#ifndef STRINGUTIL_H_
#define STRINGUTIL_H_

#include <cstring>
#include <iconv.h>
#ifdef _WIN32
#pragma comment(lib,"iconv.lib")
#endif

int code_convert(const char *from_charset,const char *to_charset,const char *inbuf, size_t inlen,char *outbuf, size_t outlen) {
 iconv_t cd;
 const char **pin = &inbuf;
 char **pout = &outbuf;

 cd = iconv_open(to_charset,from_charset);
 if (cd==0) return -1;
 memset(outbuf,0,outlen);
 iconv(cd, const_cast<char**>(pin), &inlen,pout, &outlen);
 iconv_close(cd);
 return 0;
}

/* UTF-8 to GBK  */
int u2g(const char *inbuf, size_t inlen, char *outbuf, size_t outlen) {
 return code_convert("UTF-8","GBK",inbuf,inlen,outbuf,outlen);
}

/* GBK to UTF-8 */
int g2u(const char *inbuf, size_t inlen, char *outbuf, size_t outlen) {
 return code_convert("GBK", "UTF-8", inbuf, inlen, outbuf, outlen);
}

#endif /* STRINGUTIL_H_ */
