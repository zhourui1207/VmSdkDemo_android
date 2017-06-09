#include "md5.h"
#include "authTools.h"

#define HASHLEN 16
#define HASHHEXLEN 32

static void CvtHex(
				   char* Bin,
				   char* Hex
				   )
{
	unsigned short i;
	unsigned char j;

	for (i = 0; i < HASHLEN; i++) {
		j = (Bin[i] >> 4) & 0xf;
		if (j <= 9)
			Hex[i*2] = (j + '0');
		else
			Hex[i*2] = (j + 'a' - 10);
		j = Bin[i] & 0xf;
		if (j <= 9)
			Hex[i*2+1] = (j + '0');
		else
			Hex[i*2+1] = (j + 'a' - 10);
	};
	Hex[HASHHEXLEN] = '\0';
};

static void domd5(const char* input, size_t inputlen, char* digest)
{
	MD5_CTX ctx;
	MD5Init(&ctx);
	MD5Update(&ctx, (const unsigned char*)input, inputlen);
	MD5Final((unsigned char*)digest, &ctx);
}


/* calculate H(A1) as per spec */
static void DigestCalcHA1(
				   const char* pszAlg,
				   const char* pszUserName,
				   const char* pszRealm,
				   const char* pszPassword,
				   const char* pszNonce,
				   const char* pszCNonce,
				   char* SessionKey
				   )
{
	MD5_CTX Md5Ctx;
	unsigned char HA1[16];

	MD5Init(&Md5Ctx);
	MD5Update(&Md5Ctx, (unsigned char*)pszUserName, strlen(pszUserName));
	MD5Update(&Md5Ctx, (unsigned char*)":", 1);
	MD5Update(&Md5Ctx, (unsigned char*)pszRealm, strlen(pszRealm));
	MD5Update(&Md5Ctx, (unsigned char*)":", 1);
	MD5Update(&Md5Ctx, (unsigned char*)pszPassword, strlen(pszPassword));
	MD5Final((unsigned char*)HA1, &Md5Ctx);
	if (strcmp(pszAlg, "md5-sess") == 0)
	{
		MD5Init(&Md5Ctx);
		MD5Update(&Md5Ctx, (unsigned char*)HA1, HASHLEN);
		MD5Update(&Md5Ctx, (unsigned char*)":", 1);
		MD5Update(&Md5Ctx, (unsigned char*)pszNonce, strlen(pszNonce));
		MD5Update(&Md5Ctx, (unsigned char*)":", 1);
		MD5Update(&Md5Ctx, (unsigned char*)pszCNonce, strlen(pszCNonce));
		MD5Final((unsigned char*)HA1, &Md5Ctx);
	}
	CvtHex((char*)HA1, SessionKey);
}


/* calculate request-digest/response-digest as per HTTP Digest spec */
static void DigestCalcResponse(
						const char* HA1,           /* H(A1) */
						const char* pszNonce,       /* nonce from server */
						const char* pszNonceCount,  /* 8 hex digits */
						const char* pszCNonce,      /* client nonce */
						const char* pszQop,         /* qop-value: "", "auth", "auth-int" */
						const char* pszMethod,      /* method from the request */
						const char* pszDigestUri,   /* requested URL */
						const char* HEntity,       /* H(entity body) if qop="auth-int" */
						char* Response      /* request-digest or response-digest */
						)
{
	MD5_CTX Md5Ctx;
	char HA2[16];
	char RespHash[16];
	char HA2Hex[32+1];

	// calculate H(A2)
	MD5Init(&Md5Ctx);
	MD5Update(&Md5Ctx, (unsigned char*)pszMethod, strlen(pszMethod));
	MD5Update(&Md5Ctx, (unsigned char*)":", 1);
	MD5Update(&Md5Ctx, (unsigned char*)pszDigestUri, strlen(pszDigestUri));
	if (strcmp(pszQop, "auth-int") == 0)
	{
		MD5Update(&Md5Ctx, (unsigned char*)":", 1);
		MD5Update(&Md5Ctx, (unsigned char*)HEntity, HASHHEXLEN);
	}
	MD5Final((unsigned char*)HA2, &Md5Ctx);
	CvtHex(HA2, HA2Hex);

	// calculate response
	MD5Init(&Md5Ctx);
	MD5Update(&Md5Ctx, (unsigned char*)HA1, HASHHEXLEN);
	MD5Update(&Md5Ctx, (unsigned char*)":", 1);
	MD5Update(&Md5Ctx, (unsigned char*)pszNonce, strlen(pszNonce));
	MD5Update(&Md5Ctx, (unsigned char*)":", 1);
	if (*pszQop)
	{
		MD5Update(&Md5Ctx, (unsigned char*)pszNonceCount, strlen(pszNonceCount));
		MD5Update(&Md5Ctx, (unsigned char*)":", 1);
		MD5Update(&Md5Ctx, (unsigned char*)pszCNonce, strlen(pszCNonce));
		MD5Update(&Md5Ctx, (unsigned char*)":", 1);
		MD5Update(&Md5Ctx, (unsigned char*)pszQop, strlen(pszQop));
		MD5Update(&Md5Ctx, (unsigned char*)":", 1);
	}
	MD5Update(&Md5Ctx, (unsigned char*)HA2Hex, HASHHEXLEN);
	MD5Final((unsigned char*)RespHash, &Md5Ctx);
	CvtHex(RespHash, Response);
}

//
//void AuthorTest()
//{
//	char* username="admin";
//	char* passwd = "12345";
//	char* realm="4419b7130109";
//	char* nonce="cb45a273b41c5e577f5baa1471229afc";
//	//char* uri="rtsp://192.168.1.247:554/Streaming/Channels/1?transportmode=unicast&profile=Profile_1";
//	char* uri="rtsp://192.168.1.247:554/Streaming/Channels/1/";
	
//
//	char HA1[32+1], HRes[32+1];
//
//	{
//		DigestCalcHA1("", username, realm, passwd, "", "", HA1);
//		DigestCalcResponse(HA1, nonce, "", "", "", "SETUP", uri, "", HRes);
//	}
//
//	ComputeResponse(HRes, "SETUP", username, passwd, realm, nonce, uri);
//}

void ComputeResponse(char* pResonse, const char* pMethodName, const char* pUserName, const char* pPasswd, const char* pRealm, const char* pNonce, const char* pUri)
{	
	char HA1[32+1];

	DigestCalcHA1("", pUserName, pRealm, pPasswd, "", "", HA1);

	DigestCalcResponse(HA1, pNonce, "", "", "", pMethodName, pUri, "", pResonse);
}
