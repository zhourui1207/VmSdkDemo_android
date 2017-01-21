//
// Created by zhou rui on 16/11/2.
//

#ifdef PHONE_PLATEFORM  // 如果是手机操作平台

#include "VmPlayer.h"
#include "../encdec/Decoder.h"
#include "../render/EGLRender.h"

VMPLAYER_API bool CALL_METHOD VmPlayer_DecoderInit(unsigned payloadType, long &decoderHandle) {
  Dream::Decoder *pDecoder = new Dream::Decoder();
  if (!pDecoder->Init(payloadType)) {
    delete pDecoder;
    return false;
  }
  decoderHandle = (long) pDecoder;
  return true;
}

VMPLAYER_API void CALL_METHOD VmPlayer_DecoderUninit(long decoderHandle) {
  Dream::Decoder *pDecoder = (Dream::Decoder *) decoderHandle;
  if (pDecoder == nullptr) {
    return;
  }
  pDecoder->Uninit();
  delete pDecoder;
}

VMPLAYER_API bool CALL_METHOD VmPlayer_DecodeNalu2RGB(long decoderHandle, const char *inData,
                                                      int inLen, char *outData, int &outLen,
                                                      int &width, int &height, int &framerate) {
  Dream::Decoder *pDecoder = (Dream::Decoder *) decoderHandle;
  if (pDecoder == nullptr) {
    return false;
  }
  return pDecoder->DecodeNalu2RGB(inData, inLen, outData, outLen, width, height, framerate);
}

VMPLAYER_API bool CALL_METHOD VmPlayer_DecodeNalu2YUV(long decoderHandle, const char *inData,
                                                      int inLen, char *outYData, int &outYLen,
                                                      char *outUData, int &outULen, char *outVData,
                                                      int &outVLen, int &width, int &height, int &framerate) {
  Dream::Decoder *pDecoder = (Dream::Decoder *) decoderHandle;
  if (pDecoder == nullptr) {
    return false;
  }
  return pDecoder->DecodeNalu2YUV(inData, inLen, outYData, outYLen, outUData, outULen, outVData,
                                  outVLen, width, height, framerate);
}

VMPLAYER_API int CALL_METHOD VmPlayer_GetFrameWidth(long decoderHandle) {
  Dream::Decoder *pDecoder = (Dream::Decoder *) decoderHandle;
  if (pDecoder == nullptr) {
    return 0;
  }
  return pDecoder->Width();
}

VMPLAYER_API int CALL_METHOD VmPlayer_GetFrameHeight(long decoderHandle) {
  Dream::Decoder *pDecoder = (Dream::Decoder *) decoderHandle;
  if (pDecoder == nullptr) {
    return 0;
  }
  return pDecoder->Height();
}

VMPLAYER_API bool CALL_METHOD VmPlayer_RenderInit(void *nativeWindow, long &renderHandle) {
  Dream::EGLRender *pRender = new Dream::EGLRender();
  if (!pRender->Init(nativeWindow)) {
    delete pRender;
    return false;
  }
  renderHandle = (long) pRender;
  return true;
}

VMPLAYER_API void CALL_METHOD VmPlayer_RenderUninit(long renderHandle) {
  Dream::EGLRender *pRender = (Dream::EGLRender *) renderHandle;
  if (pRender == nullptr) {
    return;
  }
  pRender->Uninit();
  delete pRender;
}

VMPLAYER_API void CALL_METHOD VmPlayer_RenderChangeSize(long renderHandle, int width, int height) {
  Dream::EGLRender *pRender = (Dream::EGLRender *) renderHandle;
  if (pRender == nullptr) {
    return;
  }
  pRender->ChangeSize(width, height);
}

VMPLAYER_API bool CALL_METHOD VmPlayer_RenderDrawYUV(long renderHandle, const char *yData, int yLen,
                                                     const char *uData, int uLen, const char *vData,
                                                     int vLen, int width, int height) {
  Dream::EGLRender *pRender = (Dream::EGLRender *) renderHandle;
  if (pRender == nullptr) {
    return false;
  }
  return pRender->DrawYUV(yData, yLen, uData, uLen, vData, vLen, width, height);
}

VMPLAYER_API bool CALL_METHOD VmPlayer_RenderOffScreenRendering(long renderHandle,
                                                                const char *yData, int yLen,
                                                                const char *uData, int uLen,
                                                                const char *vData,
                                                                int vLen, int width, int height,
                                                                char *outRgbData,
                                                                int &outRgbLen) {
  Dream::EGLRender *pRender = (Dream::EGLRender *) renderHandle;
  if (pRender == nullptr) {
    return false;
  }
  return pRender->OffScreenRendering(yData, yLen, uData, uLen, vData, vLen, width, height,
                                     outRgbData, outRgbLen);
}

#endif
