#ifndef _AUTH_TOOLS_H_
#define _AUTH_TOOLS_H_

void
ComputeResponse(char *pResonse, const char *pMethodName, const char *pUserName, const char *pPasswd,
                const char *pRealm, const char *pNonce, const char *pUri);

#endif
