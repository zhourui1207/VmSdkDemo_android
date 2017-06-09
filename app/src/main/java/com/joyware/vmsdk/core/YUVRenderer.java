package com.joyware.vmsdk.core;

import android.graphics.ImageFormat;
import android.graphics.Rect;
import android.graphics.YuvImage;
import android.os.AsyncTask;
import android.util.Log;

import com.joyware.vmsdk.CaptureCallback;
import com.joyware.vmsdk.util.StringUtil;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;

/**
 * Created by zhourui on 17/4/29.
 * email: 332867864@qq.com
 * phone: 17006421542
 */

public class YUVRenderer implements GLRenderer {

    private static final String TAG = "YUVRenderer";

    // yuv帧数据
    private int mVideoWidth, mVideoHeight;
    private ByteBuffer mY;
    private ByteBuffer mU;
    private ByteBuffer mV;

    // 截图使用
    byte[] mYuvImageData;
    byte[] mUvTmpBuf;
    byte[] mNv21UvBuf;

    private boolean mScaleEnable;
    private float mLeftPercent = 0f;
    private float mTopPercent = 0f;
    private float mWidthPercent = 1f;
    private float mHeightPercent = 1f;

    private GLRenderSurface mRenderSurface;

    private int mScreenWidth, mScreenHeight;

    public GLRenderSurface getRenderSurface() {
        return mRenderSurface;
    }

    public void setRenderSurface(GLRenderSurface renderSurface) {
        if (mRenderSurface != renderSurface && mRenderSurface != null) {
            mRenderSurface.exit();
            mRenderSurface.setRenderer(null);
            mRenderSurface = null;
        }
        mRenderSurface = renderSurface;
        if (mRenderSurface != null) {
            mRenderSurface.setRenderer(this);
            mRenderSurface.start();
        }
    }

    @Override
    public void onSizeChanged(long renderHandle, int w, int h) {
        mScreenWidth = w;
        mScreenHeight = h;
        synchronized (this) {
            GLHelper.scaleTo(renderHandle, mScaleEnable, Math.round(mLeftPercent * mScreenWidth),
                    Math.round(mTopPercent * mScreenHeight), Math.round(mWidthPercent *
                            mScreenWidth), Math.round(mHeightPercent * mScreenHeight));
        }
    }

    @Override
    public int onDrawFrame(long renderHandle) {
        if (mY != null && renderHandle != 0) {
            synchronized (this) {
                return GLHelper.drawYUV(renderHandle, mY.array(), 0, mY.array().length, mU.array
                                (), 0, mU.array().length, mV.array(), 0, mV.array().length,
                        mVideoWidth, mVideoHeight);
            }
        }
        return 0;
    }

    /**
     * this method will be called from native code, it's used for passing yuv mData to me.
     */
    public void frameChanged(int w, int h, byte[] yData, int yStart, int yLen, byte[]
            uData, int uStart, int uLen, byte[] vData, int vStart, int vLen) {
        synchronized (this) {
            if (w > 0 && h > 0) {
                if (w != mVideoWidth || h != mVideoHeight) {
                    this.mVideoWidth = w;
                    this.mVideoHeight = h;
                    int yarraySize = w * h;
                    int uvarraySize = yarraySize / 4;
                    mY = ByteBuffer.allocate(yarraySize);
                    mU = ByteBuffer.allocate(uvarraySize);
                    mV = ByteBuffer.allocate(uvarraySize);
                    mYuvImageData = new byte[yarraySize * 3 / 2];
                    mUvTmpBuf = new byte[uvarraySize * 2];
                    mNv21UvBuf = new byte[uvarraySize * 2];
                }
            }
            mY.clear();
            mU.clear();
            mV.clear();
            mY.put(yData, yStart, yLen);
            mU.put(uData, uStart, uLen);
            mV.put(vData, vStart, vLen);
        }
        // request to render
        if (mRenderSurface != null) {
            mRenderSurface.requestRender();
        }
    }

    public void scaleChanged(boolean scaleEnable, float leftPercent, float topPercent, float
            widthPercent, float heightPercent) {
        synchronized (this) {
            if (scaleEnable) {
                mScaleEnable = true;
                mLeftPercent = leftPercent;
                mTopPercent = topPercent;
                mWidthPercent = widthPercent;
                mHeightPercent = heightPercent;
            } else {
                mScaleEnable = false;
                mLeftPercent = mTopPercent = 0f;
                mWidthPercent = mHeightPercent = 1f;
            }
        }
        if (mRenderSurface != null) {
            mRenderSurface.surfaceChanged(null, 0, mScreenWidth, mScreenHeight);
        }
    }

    public boolean capture(String fileName, CaptureCallback captureCallback) {
        if (captureCallback != null) {
            new ScreenshotTask(new WeakReference<YUVRenderer>(this), fileName, captureCallback).executeOnExecutor(AsyncTask
                    .THREAD_POOL_EXECUTOR);
        } else {
            return captureAndSave(fileName);
        }

        return true;
    }

    private boolean captureAndSave(String fileName) {
        int width;
        int height;
        synchronized (this) {
            if (mY == null || mU == null || mV == null || mVideoWidth <= 0 || mVideoHeight <= 0) {
                return false;
            }

            width = mVideoWidth;
            height = mVideoHeight;
            int yLen = mVideoWidth * mVideoHeight;
            int uvLen = mVideoWidth * mVideoHeight / 4;
            mUvTmpBuf = new byte[uvLen * 2];
            mNv21UvBuf = new byte[uvLen * 2];
            System.arraycopy(mY.array(), 0, mYuvImageData, 0, yLen);
            System.arraycopy(mU.array(), 0, mUvTmpBuf, 0, uvLen);
            System.arraycopy(mV.array(), 0, mUvTmpBuf, uvLen, uvLen);

            YUVP2NV21(mUvTmpBuf, 0, uvLen * 2, mNv21UvBuf);

            System.arraycopy(mNv21UvBuf, 0, mYuvImageData, yLen, uvLen * 2);
        }

        if (mYuvImageData != null && width > 0 && height > 0) {
            FileOutputStream fileOutputStream = null;
            if (fileName != null && !fileName.isEmpty()) {
                try {
                    fileOutputStream = new FileOutputStream(fileName);
                } catch (FileNotFoundException e) {
                    Log.e(TAG, StringUtil.getStackTraceAsString(e));
                    return false;
                }
            }

            YuvImage yuvImage = new YuvImage(mYuvImageData, ImageFormat.NV21, width, height, null);
            if (yuvImage.compressToJpeg(new Rect(0, 0, width, height), 100, fileOutputStream)) {
                Log.w(TAG, "screenshotAndSave success");
                return true;
            }
        }
        return false;
    }

    private static class ScreenshotTask extends AsyncTask<Void, Void, Boolean> {
        private WeakReference<YUVRenderer> mYUVRendererWeakReference;

        private String mFileName;
        private CaptureCallback mCaptureCallback;

        private ScreenshotTask() {
        }

        private ScreenshotTask(WeakReference<YUVRenderer> YUVRendererWeakReference, String fileName,
                              CaptureCallback captureCallback) {
            mYUVRendererWeakReference = YUVRendererWeakReference;
            mFileName = fileName;
            mCaptureCallback = captureCallback;
        }

        @Override
        protected Boolean doInBackground(Void... params) {
            YUVRenderer yuvRenderer = mYUVRendererWeakReference.get();

            if (yuvRenderer == null || this.mFileName == null || this.mFileName.isEmpty()) {
                return null;
            }

            return yuvRenderer.captureAndSave(mFileName);
        }

        @Override
        protected void onPostExecute(Boolean aBoolean) {
            if (this.mCaptureCallback != null) {
                if (aBoolean) {
                    this.mCaptureCallback.onSuccess(this.mFileName);
                } else {
                    this.mCaptureCallback.onFailed("");
                }
            }
        }
    }

    private static native void YUVP2NV21(byte[] inData, int inStart, int inLen, byte[] outData);
}
