//
// Created by zhou rui on 16/11/2.
//

#ifndef VMPLAYER_H
#define VMPLAYER_H

#ifdef PHONE_PLATEFORM  // 如果是手机操作平台

#define VMPLAYER_API
#define CALL_METHOD
#define out

#ifdef __cplusplus
extern "C" {
#endif

VMPLAYER_API bool CALL_METHOD VmPlayer_DecoderInit(unsigned payloadType, long &decoderHandle);
VMPLAYER_API void CALL_METHOD VmPlayer_DecoderUninit(long decoderHandle);
VMPLAYER_API bool CALL_METHOD VmPlayer_DecodeNalu2RGB(long decoderHandle, const char *inData,
                                                      int inLen, char *outData, int &outLen,
                                                      int &width, int &height, int &framerate);
VMPLAYER_API bool CALL_METHOD VmPlayer_DecodeNalu2YUV(long decoderHandle, const char *inData,
                                                      int inLen, char *outYData, int &outYLen,
                                                      char *outUData, int &outULen, char *outVData,
                                                      int &outVLen, int &width, int &height, int &framerate);
VMPLAYER_API int CALL_METHOD VmPlayer_GetFrameWidth(long decoderHandle);
VMPLAYER_API int CALL_METHOD VmPlayer_GetFrameHeight(long decoderHandle);

VMPLAYER_API bool CALL_METHOD VmPlayer_RenderInit(void *nativeWindow, long &renderHandle);
VMPLAYER_API void CALL_METHOD VmPlayer_RenderUninit(long renderHandle);
VMPLAYER_API void CALL_METHOD VmPlayer_RenderChangeSize(long renderHandle, int width, int height);
VMPLAYER_API bool CALL_METHOD VmPlayer_RenderDrawYUV(long renderHandle, const char *yData, int yLen,
                                                     const char *uData, int uLen, const char *vData,
                                                     int vLen, int width, int height);
VMPLAYER_API bool CALL_METHOD VmPlayer_RenderOffScreenRendering(long renderHandle,
                                                                const char *yData, int yLen,
                                                                const char *uData, int uLen,
                                                                const char *vData,
                                                                int vLen, int width, int height,
                                                                char *outRgbData,
                                                                int &outRgbLen);
#ifdef __cplusplus
}
#endif


#endif

#endif
