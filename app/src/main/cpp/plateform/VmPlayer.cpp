//
// Created by zhou rui on 16/11/2.
//

#ifdef PHONE_PLATEFORM  // 如果是手机操作平台

#include "VmPlayer.h"
#include "../encdec/Decoder.h"
#include "../render/EGLRender.h"
#include "../encdec/AACEncoder.h"

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
                                                      int &outVLen, int &width, int &height,
                                                      int &framerate) {
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

VMPLAYER_API bool CALL_METHOD VmPlayer_ACCEncoderInit(int samplerate, long &encoderHandle) {
    Dream::AACEncoder *pEncoder = new Dream::AACEncoder();
    if (!pEncoder->Init(samplerate)) {
        delete pEncoder;
        return false;
    }
    encoderHandle = (long) pEncoder;
    return true;
}

VMPLAYER_API void CALL_METHOD VmPlayer_ACCEncoderUninit(long decoderHandle) {
    Dream::AACEncoder *pEncoder = (Dream::AACEncoder *) decoderHandle;
    if (pEncoder == nullptr) {
        return;
    }
    pEncoder->Uninit();
    delete pEncoder;
}

VMPLAYER_API bool CALL_METHOD VmPlayer_AACEncodePCM2AAC(long decoderHandle, const char *inData,
                                                        int inLen, char *outData, int &outLen) {
    Dream::AACEncoder *pEncoder = (Dream::AACEncoder *) decoderHandle;
    if (pEncoder == nullptr) {
        return false;
    }
    return pEncoder->EncodePCM2AAC(inData, inLen, outData, outLen);
}

VMPLAYER_API bool CALL_METHOD VmPlayer_RenderInit(long &renderHandle) {
    Dream::EGLRender *pRender = new Dream::EGLRender();
    renderHandle = (long) pRender;
    return true;
}

VMPLAYER_API void CALL_METHOD VmPlayer_RenderUninit(long renderHandle) {
    Dream::EGLRender *pRender = (Dream::EGLRender *) renderHandle;
    if (pRender == nullptr) {
        return;
    }
    delete pRender;
}

VMPLAYER_API bool CALL_METHOD VmPlayer_RenderStart(long renderHandle) {
    Dream::EGLRender *pRender = (Dream::EGLRender *) renderHandle;
    if (pRender != nullptr) {
        return pRender->Start();
    }
    return false;
}

VMPLAYER_API void CALL_METHOD VmPlayer_RenderFinish(long renderHandle) {
    Dream::EGLRender *pRender = (Dream::EGLRender *) renderHandle;
    if (pRender == nullptr) {
        return;
    }
    pRender->Finish();
}

VMPLAYER_API bool CALL_METHOD VmPlayer_RenderCreateSurface(long renderHandle, void *nativeWindow) {
    Dream::EGLRender *pRender = (Dream::EGLRender *) renderHandle;
    if (pRender != nullptr) {
        return pRender->CreateSurface(nativeWindow);
    }
    return false;
}

VMPLAYER_API void CALL_METHOD VmPlayer_RenderDestroySurface(long renderHandle) {
    Dream::EGLRender *pRender = (Dream::EGLRender *) renderHandle;
    if (pRender == nullptr) {
        return;
    }
    pRender->DestroySurface();
}

VMPLAYER_API void CALL_METHOD VmPlayer_RenderSurfaceCreated(long renderHandle) {
    Dream::EGLRender *pRender = (Dream::EGLRender *) renderHandle;
    if (pRender == nullptr) {
        return;
    }
    pRender->SurfaceCreated();
}

VMPLAYER_API void CALL_METHOD VmPlayer_RenderSurfaceDestroyed(long renderHandle) {
    Dream::EGLRender *pRender = (Dream::EGLRender *) renderHandle;
    if (pRender == nullptr) {
        return;
    }
    pRender->SurfaceDestroyed();
}

VMPLAYER_API void CALL_METHOD
VmPlayer_RenderSurfaceChanged(long renderHandle, int width, int height) {
    Dream::EGLRender *pRender = (Dream::EGLRender *) renderHandle;
    if (pRender == nullptr) {
        return;
    }
    pRender->SurfaceChanged(width, height);
}

VMPLAYER_API void CALL_METHOD
VmPlayer_RenderScaleTo(long renderHandle, bool scaleEnable, int left, int top,
                       int width, int height) {
    Dream::EGLRender *pRender = (Dream::EGLRender *) renderHandle;
    if (pRender == nullptr) {
        return;
    }
    pRender->ScaleTo(scaleEnable, left, top, width, height);
}

VMPLAYER_API int CALL_METHOD VmPlayer_RenderDrawYUV(long renderHandle, const char *yData, int yLen,
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
