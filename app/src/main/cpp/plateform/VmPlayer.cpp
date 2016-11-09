//
// Created by zhou rui on 16/11/2.
//

#include "VmPlayer.h"
#include "../encdec/Decoder.h"

VMPLAYER_API bool CALL_METHOD VmPlayer_DecoderInit(unsigned payloadType, bool isRGB, long &decoderHandle, void* nativeWindowType) {
  Dream::Decoder* pDecoder = new Dream::Decoder();
  if (!pDecoder->Init(payloadType, isRGB, nativeWindowType)) {
    delete pDecoder;
    return false;
  }
  decoderHandle = (long)pDecoder;
  return true;
}

VMPLAYER_API void CALL_METHOD VmPlayer_DecoderUninit(long decoderHandle) {
  Dream::Decoder* pDecoder = (Dream::Decoder*) decoderHandle;
  if (pDecoder == nullptr) {
    return;
  }
  pDecoder->Uninit();
  delete pDecoder;
}

VMPLAYER_API bool CALL_METHOD VmPlayer_DecodeNalu(long decoderHandle, const char *inData,
                                                  int inLen, char *outData, int &outLen) {
  Dream::Decoder* pDecoder = (Dream::Decoder*) decoderHandle;
  if (pDecoder == nullptr) {
    return false;
  }
  return pDecoder->DecodeNalu(inData, inLen, outData, outLen);
}

VMPLAYER_API int CALL_METHOD VmPlayer_GetFrameWidth(long decoderHandle) {
  Dream::Decoder* pDecoder = (Dream::Decoder*) decoderHandle;
  if (pDecoder == nullptr) {
    return 0;
  }
  return pDecoder->Width();
}
VMPLAYER_API int CALL_METHOD VmPlayer_GetFrameHeight(long decoderHandle) {
  Dream::Decoder* pDecoder = (Dream::Decoder*) decoderHandle;
  if (pDecoder == nullptr) {
    return 0;
  }
  return pDecoder->Height();
}