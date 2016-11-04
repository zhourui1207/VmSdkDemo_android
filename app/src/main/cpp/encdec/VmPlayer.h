//
// Created by zhou rui on 16/11/2.
//

#ifndef VMPLAYER_H
#define VMPLAYER_H

#define VMPLAYER_API
#define CALL_METHOD
#define out

#ifdef __cplusplus
extern "C" {
#endif

VMPLAYER_API bool CALL_METHOD VmPlayer_DecoderInit(unsigned payloadType, bool isRGB, long &decoderHandle);
VMPLAYER_API void CALL_METHOD VmPlayer_DecoderUninit(long decoderHandle);
VMPLAYER_API bool CALL_METHOD VmPlayer_DecodeNalu(long decoderHandle, const char *inData,
                                                  int inLen, char *outData, int &outLen);
VMPLAYER_API int CALL_METHOD VmPlayer_GetFrameWidth(long decoderHandle);
VMPLAYER_API int CALL_METHOD VmPlayer_GetFrameHeight(long decoderHandle);
#ifdef __cplusplus
}
#endif


#endif
