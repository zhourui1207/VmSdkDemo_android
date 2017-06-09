package com.joyware.vmsdk.core;

import android.view.Surface;

/**
 * Created by zhourui on 17/4/29.
 * email: 332867864@qq.com
 * phone: 17006421542
 */

public class GLHelper {

    public static long init() {
        return nativeInit();
    }

    public static void uninit(long renderHandle) {
        nativeUninit(renderHandle);
    }

    public static boolean start(long renderHandle) {
        return nativeStart(renderHandle);
    }

    public static void finish(long renderHandle) {
        nativeFinish(renderHandle);
    }

    public static boolean createSurface(long renderHandle, Surface surface) {
        return nativeCreateSurface(renderHandle, surface);
    }

    public static void destroySurface(long renderHandle) {
        nativeDestroySurface(renderHandle);
    }

    public static void surfaceChanged(long renderHandle, int width, int height) {
        nativeSurfaceChanged(renderHandle, width, height);
    }

    public static void scaleTo(long renderHandle, boolean enable, int left, int
            top, int width, int height) {
        nativeScaleTo(renderHandle, enable, left, top, width, height);
    }

    public static int drawYUV(long renderHandle, byte[] yData, int yStart, int yLen,
                              byte[] uData, int uStart, int uLen, byte[] vData, int
                                      vStart, int vLen, int width, int height) {
        return nativeDrawYUV(renderHandle, yData, yStart, yLen, uData, uStart, uLen, vData,
                vStart, vLen, width, height);
    }

    private static native long nativeInit();

    private static native void nativeUninit(long renderHandle);

    private static native boolean nativeStart(long renderHandle);

    private static native void nativeFinish(long renderHandle);

    private static native boolean nativeCreateSurface(long renderHandle, Surface surface);

    private static native void nativeDestroySurface(long renderHandle);

    private static native void nativeSurfaceCreated(long renderHandle);

    private static native void nativeSurfaceDestroyed(long renderHandle);

    private static native void nativeSurfaceChanged(long renderHandle, int width, int height);

    private static native void nativeScaleTo(long renderHandle, boolean enable, int left, int
            top, int width, int height);

    private static native int nativeDrawYUV(long renderHandle, byte[] yData, int yStart, int yLen,
                                            byte[] uData, int uStart, int uLen, byte[] vData, int
                                                    vStart, int vLen, int width, int height);

}
