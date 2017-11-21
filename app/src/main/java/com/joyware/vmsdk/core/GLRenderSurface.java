package com.joyware.vmsdk.core;

import android.graphics.Rect;
import android.support.annotation.NonNull;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;

import com.joyware.vmsdk.util.SafeUtil;

import java.lang.ref.WeakReference;

/**
 * Created by zhourui on 17/4/28.
 * email: 332867864@qq.com
 * phone: 17006421542
 * opengl渲染线程
 */

public class GLRenderSurface implements SurfaceHolder.Callback2 {

    private final static String TAG = "GLRenderSurface";
    private final static boolean LOG = false;

    private GLThread mGLRenderThread;  // gl渲染线程
    private SurfaceHolder mSurfaceHolder;
    private GLRenderer mRenderer;

    public final static int RENDERMODE_WHEN_DIRTY = 0;  // 需求模式
    public final static int RENDERMODE_CONTINUOUSLY = 1;  // 连续模式

    private GLRenderSurface() {

    }

    public GLRenderSurface(SurfaceHolder surfaceHolder) {
        mSurfaceHolder = SafeUtil.checkNotNull(surfaceHolder);
    }

    public SurfaceHolder getSurfaceHolder() {
        return mSurfaceHolder;
    }

    public void setRenderer(GLRenderer renderer) {
        mRenderer = renderer;
    }

    public void setRenderMode(int renderMode) {
        if (mGLRenderThread != null) {
            mGLRenderThread.setRenderMode(renderMode);
        }
    }

    public void start() {
        if (mGLRenderThread == null) {
            mSurfaceHolder.addCallback(this);
            mGLRenderThread = new GLThread(new WeakReference<GLRenderSurface>(this));
            mGLRenderThread.start();

            Rect frame = mSurfaceHolder.getSurfaceFrame();
            if (frame != null) {
                mGLRenderThread.onWindowResize(frame.width(), frame.height());
            }

            Surface surface = mSurfaceHolder.getSurface();
            if (surface != null && surface.isValid()) {
                mGLRenderThread.surfaceCreated();
            } else {
                mGLRenderThread.surfaceDestroyed();
            }
        }
    }

    public void exit() {
        if (mGLRenderThread != null) {
            mGLRenderThread.requestExitAndWait();
        }
        mSurfaceHolder.removeCallback(this);
    }

    public void requestRender() {
        if (mGLRenderThread != null) {
            mGLRenderThread.requestRender();
        }
    }

    private static class GLThread extends Thread {
        private final String TAG = "GLThread";

        @NonNull
        WeakReference<GLRenderSurface> mGLRenderSurfaceWeakReference;

        private int mRenderMode = RENDERMODE_CONTINUOUSLY;
//        private int mRenderMode = RENDERMODE_WHEN_DIRTY;

        private long mRenderHandle;  // 渲染器句柄
        private boolean mExited;
        private boolean mShouldExit;
        private int mWidth;
        private int mHeight;
        private boolean mHasSurface;
        private boolean mFinishedCreatingEglSurface;
        private boolean mWantRenderNotification;
        private boolean mWaitingForSurface;
        private boolean mSurfaceIsBad;
        private boolean mHaveEglSurface;
        private boolean mHaveEglContext;
        private boolean mRequestRender;
        private boolean mRenderComplete;

        private boolean mSizeChanged = true;

        public GLThread(WeakReference<GLRenderSurface> GLRenderSurfaceWeakReference) {
            mGLRenderSurfaceWeakReference = SafeUtil.checkNotNull(GLRenderSurfaceWeakReference);
        }

        @Override
        public void run() {
            if (LOG) {
                Log.i(TAG, "starting tid=" + getId());
            }

            try {
                // 创建gl渲染器
                mRenderHandle = GLHelper.init();
                if (mRenderHandle == 0) {
                    if (LOG) {
                        Log.e(TAG, "renderHandle init failed!");
                    }
                    return;
                }

                guardedRun();
            } catch (InterruptedException e) {
                // fall thru and exit normally
            } finally {
                if (mRenderHandle != 0) {
                    GLHelper.uninit(mRenderHandle);
                    mRenderHandle = 0;
                }
                sGLThreadManager.threadExiting(this);
            }
        }

        private void guardedRun() throws InterruptedException {
            try {

                boolean createEglContext = false;
                boolean createEglSurface = false;
                boolean lostEglContext = false;
                boolean sizeChanged = false;
                boolean wantRenderNotification = false;
                boolean doRenderNotification = false;
                boolean askedToReleaseEglContext = false;
                int w = 0;
                int h = 0;

                while (true) {
                    synchronized (sGLThreadManager) {
                        while (true) {
                            if (mShouldExit) {
                                return;
                            }

                            // Have we lost the EGL context?
                            if (lostEglContext) {
                                stopEglSurfaceLocked();
                                stopEglContextLocked();
                                lostEglContext = false;
                            }

                            // Have we lost the SurfaceView surface?
                            if ((!mHasSurface) && (!mWaitingForSurface)) {
                                if (LOG) {
                                    Log.i("GLThread", "noticed surfaceView surface lost tid=" +
                                            getId());
                                }
                                if (mHaveEglSurface) {
                                    stopEglSurfaceLocked();
                                }
                                mWaitingForSurface = true;
                                mSurfaceIsBad = false;
                                sGLThreadManager.notifyAll();
                            }

                            // Have we acquired the surface view surface?
                            if (mHasSurface && mWaitingForSurface) {
                                if (LOG) {
                                    Log.i("GLThread", "noticed surfaceView surface acquired tid="
                                            + getId());
                                }
                                mWaitingForSurface = false;
                                sGLThreadManager.notifyAll();
                            }

                            if (doRenderNotification) {
                                if (LOG) {
                                    Log.i("GLThread", "sending render notification tid=" + getId());
                                }
                                mWantRenderNotification = false;
                                doRenderNotification = false;
                                mRenderComplete = true;
                                sGLThreadManager.notifyAll();
                            }

                            // Ready to draw?
                            if (readyToDraw()) {

                                // If we don't have an EGL context, try to acquire one.
                                if (!mHaveEglContext) {
                                    if (askedToReleaseEglContext) {
                                        askedToReleaseEglContext = false;
                                    } else {
//                                        try {
//                                            mHaveEglContext = GLHelper.start(mRenderHandle);
//                                        } catch (RuntimeException t) {
//                                            sGLThreadManager.releaseEglContextLocked(this);
//                                            throw t;
//                                        }
                                        mHaveEglContext = true;
                                        createEglContext = true;
                                        sGLThreadManager.notifyAll();
                                    }
                                }

                                if (mHaveEglContext && !mHaveEglSurface) {
                                    mHaveEglSurface = true;
                                    createEglSurface = true;
                                    sizeChanged = true;
                                }

                                if (mHaveEglSurface) {
                                    if (mSizeChanged) {
                                        sizeChanged = true;
                                        w = mWidth;
                                        h = mHeight;
                                        mWantRenderNotification = true;
                                        if (LOG) {
                                            Log.i("GLThread",
                                                    "noticing that we want render notification tid="
                                                            + getId());
                                        }

                                        // Destroy and recreate the EGL surface.
//                                        createEglSurface = true;

                                        mSizeChanged = false;
                                    }
                                    mRequestRender = false;
                                    sGLThreadManager.notifyAll();
                                    if (mWantRenderNotification) {
                                        wantRenderNotification = true;
                                    }
                                    break;
                                }
                            }

                            // By design, this is the only place in a GLThread thread where we
                            // wait().
                            if (LOG) {
                                Log.i("GLThread", "waiting tid=" + getId()
                                        + " mHaveEglContext: " + mHaveEglContext
                                        + " mHaveEglSurface: " + mHaveEglSurface
                                        + " mFinishedCreatingEglSurface: " +
                                        mFinishedCreatingEglSurface
                                        + " mHasSurface: " + mHasSurface
                                        + " mSurfaceIsBad: " + mSurfaceIsBad
                                        + " mWaitingForSurface: " + mWaitingForSurface
                                        + " mWidth: " + mWidth
                                        + " mHeight: " + mHeight
                                        + " mRequestRender: " + mRequestRender);
                            }
                            sGLThreadManager.wait();
                        }
                    }

                    if (createEglContext) {
                        if (LOG) {
                            Log.w("GLThread", "onSurfaceCreated");
                        }
                        GLRenderSurface glRenderSurface = mGLRenderSurfaceWeakReference.get();
                        if (glRenderSurface != null && glRenderSurface.mSurfaceHolder != null) {
                            GLHelper.start(mRenderHandle);
                        }
                        createEglContext = false;
                    }

                    if (createEglSurface) {
                        if (LOG) {
                            Log.w("GLThread", "egl createSurface");
                        }
                        GLRenderSurface glRenderSurface = mGLRenderSurfaceWeakReference.get();
                        if (glRenderSurface != null && glRenderSurface.mSurfaceHolder != null) {
                            if (GLHelper.createSurface(mRenderHandle, glRenderSurface
                                    .mSurfaceHolder.getSurface())) {
                                synchronized (sGLThreadManager) {
                                    mFinishedCreatingEglSurface = true;
                                    sGLThreadManager.notifyAll();
                                }
                            }
                        } else {
                            synchronized (sGLThreadManager) {
                                mFinishedCreatingEglSurface = true;
                                mSurfaceIsBad = true;
                                sGLThreadManager.notifyAll();
                            }
                            continue;
                        }
                        createEglSurface = false;
                    }

                    if (sizeChanged) {
                        if (LOG) {
                            Log.w("GLThread", "onSurfaceChanged(" + w + ", " + h + ")");
                        }
                        GLRenderSurface glRenderSurface = mGLRenderSurfaceWeakReference.get();
                        if (glRenderSurface != null) {
                            GLHelper.surfaceChanged(mRenderHandle, w, h);
                            if (glRenderSurface.mRenderer != null) {
                                glRenderSurface.mRenderer.onSizeChanged(mRenderHandle, w, h);
                            }
                        }
                        sizeChanged = false;
                    }

                    if (LOG) {
                        Log.w("GLThread", "onDrawFrame tid=" + getId());
                    }
                    int swapError = 0;
                    {
                        GLRenderSurface glRenderSurface = mGLRenderSurfaceWeakReference.get();
                        if (glRenderSurface != null && glRenderSurface.mRenderer != null) {
                            swapError = glRenderSurface.mRenderer.onDrawFrame(mRenderHandle);
                        }
                    }

                    switch (swapError) {
                        case 0:
                            mRequestRender = false;
                            break;
//                        case EGL11.EGL_CONTEXT_LOST:
//                            if (LOG) {
//                                Log.i("GLThread", "egl context lost tid=" + getId());
//                            }
//                            lostEglContext = true;
//                            break;
                        default:
                            // Other errors typically mean that the current surface is bad,
                            // probably because the SurfaceView surface has been destroyed,
                            // but we haven't been notified yet.
                            // Log the error to help developers understand why rendering stopped.
                            // EglHelper.logEglErrorAsWarning("GLThread", "eglSwapBuffers",
                            // swapError);
                            Log.e(TAG, "swapError code [" + swapError + "]");
                            lostEglContext = true;
                            synchronized (sGLThreadManager) {
                                mSurfaceIsBad = true;
                                sGLThreadManager.notifyAll();
                            }
                            break;
                    }

                    if (wantRenderNotification) {
                        doRenderNotification = true;
                        wantRenderNotification = false;
                    }
                }
            } finally {
                // 释放opengl
                synchronized (sGLThreadManager) {
                    stopEglSurfaceLocked();
                    stopEglContextLocked();
                }
            }
        }

        private void stopEglSurfaceLocked() {
            if (mHaveEglSurface) {
                mHaveEglSurface = false;
                GLHelper.destroySurface(mRenderHandle);
            }
        }

        /*
         * This private method should only be called inside a
         * synchronized(sGLThreadManager) block.
         */
        private void stopEglContextLocked() {
            if (mHaveEglContext) {
                GLHelper.finish(mRenderHandle);
                mHaveEglContext = false;
                sGLThreadManager.releaseEglContextLocked(this);
            }
        }

        public boolean ableToDraw() {
            return mHaveEglContext && mHaveEglSurface && readyToDraw();
        }

        private boolean readyToDraw() {
            return mHasSurface && (!mSurfaceIsBad)
                    && (mWidth > 0) && (mHeight > 0)
                    && (mRequestRender || (mRenderMode == RENDERMODE_CONTINUOUSLY));
        }

        public void requestRender() {
            synchronized (sGLThreadManager) {
                mRequestRender = true;
                sGLThreadManager.notifyAll();
            }
        }

        public void requestRenderAndWait() {
            synchronized (sGLThreadManager) {
                // If we are already on the GL thread, this means a client callback
                // has caused reentrancy, for example via updating the SurfaceView parameters.
                // We will return to the client rendering code, so here we don't need to
                // do anything.
                if (Thread.currentThread() == this) {
                    return;
                }

                if (LOG) {
                    Log.i("GLThread", "requestRenderAndWait tid=" + getId());
                }

                mWantRenderNotification = true;
                mRequestRender = true;
                mRenderComplete = false;

                sGLThreadManager.notifyAll();

                while (!mExited && !mRenderComplete && ableToDraw()) {
                    try {
                        sGLThreadManager.wait();
                    } catch (InterruptedException ex) {
                        Thread.currentThread().interrupt();
                    }
                }

            }
        }

        public void surfaceCreated() {
            synchronized (sGLThreadManager) {
                if (LOG) {
                    Log.i("GLThread", "surfaceCreated tid=" + getId());
                }
                mHasSurface = true;
                mFinishedCreatingEglSurface = false;
                sGLThreadManager.notifyAll();
                while (mWaitingForSurface
                        && !mFinishedCreatingEglSurface
                        && !mExited) {
                    try {
                        sGLThreadManager.wait();
                    } catch (InterruptedException e) {
                        Thread.currentThread().interrupt();
                    }
                }
            }
        }

        public void surfaceDestroyed() {
            synchronized (sGLThreadManager) {
                if (LOG) {
                    Log.i("GLThread", "surfaceDestroyed tid=" + getId());
                }
                mHasSurface = false;
                sGLThreadManager.notifyAll();
                while ((!mWaitingForSurface) && (!mExited)) {
                    try {
                        sGLThreadManager.wait();
                    } catch (InterruptedException e) {
                        Thread.currentThread().interrupt();
                    }
                }
            }
        }

        public void onWindowResize(int w, int h) {
            synchronized (sGLThreadManager) {
                if (LOG) {
                    Log.i("GLThread", "onWindowResize tid=" + getId() + ", w=" + w + ", h=" + h);
                }
                mWidth = w;
                mHeight = h;
                mSizeChanged = true;
                mRequestRender = true;
                mRenderComplete = false;

                // If we are already on the GL thread, this means a client callback
                // has caused reentrancy, for example via updating the SurfaceView parameters.
                // We need to process the size change eventually though and update our EGLSurface.
                // So we set the parameters and return so they can be processed on our
                // next iteration.
                if (Thread.currentThread() == this) {
                    return;
                }

                sGLThreadManager.notifyAll();

                // Wait for thread to react to resize and render a frame
                while (!mExited && !mRenderComplete
                        && ableToDraw()) {
                    if (LOG) {
                        Log.i("Main thread", "onWindowResize waiting for render complete from " +
                                "tid=" + getId());
                    }
                    try {
                        sGLThreadManager.wait();
                    } catch (InterruptedException ex) {
                        Thread.currentThread().interrupt();
                    }
                }
            }
        }


        public void requestExitAndWait() {
            // don't call this from GLThread thread or it is a guaranteed
            // deadlock!
            synchronized (sGLThreadManager) {
                mShouldExit = true;
                sGLThreadManager.notifyAll();
                while (!mExited) {
                    try {
                        sGLThreadManager.wait();
                    } catch (InterruptedException ex) {
                        Thread.currentThread().interrupt();
                    }
                }
            }
        }

        public void setRenderMode(int renderMode) {
            if (!((RENDERMODE_WHEN_DIRTY <= renderMode) && (renderMode <=
                    RENDERMODE_CONTINUOUSLY))) {
                throw new IllegalArgumentException("renderMode");
            }
            synchronized (sGLThreadManager) {
                mRenderMode = renderMode;
                sGLThreadManager.notifyAll();
            }
        }

        public int getRenderMode() {
            synchronized (sGLThreadManager) {
                return mRenderMode;
            }
        }
    }


    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        if (mGLRenderThread != null) {
            mGLRenderThread.surfaceCreated();
        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        if (mGLRenderThread != null) {
//            Log.w(TAG, "SurfaceChanged surfaceHolder:" + holder + ", format:" + format + ", " +
//                    "width:" + width + ", height:" + height);
            mGLRenderThread.onWindowResize(width, height);
        }
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        if (mGLRenderThread != null) {
            mGLRenderThread.surfaceDestroyed();
        }
    }

    @Override
    public void surfaceRedrawNeeded(SurfaceHolder holder) {
        // 这个重写非常有用，假如renderer中的数据没有更新，但是变化了serface的宽高，那么会触发这个进行刷新视图
        if (mGLRenderThread != null) {
            mGLRenderThread.requestRenderAndWait();
        }
    }

    private static class GLThreadManager {
        private final static String TAG = "GLThreadManager";

        public synchronized void threadExiting(GLThread thread) {
            if (LOG) {
                Log.i("GLThread", "exiting tid=" + thread.getId());
            }
            thread.mExited = true;
            notifyAll();
        }

        /*
         * Releases the EGL context. Requires that we are already in the
         * sGLThreadManager monitor when this is called.
         */
        public void releaseEglContextLocked(GLThread thread) {
            notifyAll();
        }
    }

    @NonNull
    private static final GLThreadManager sGLThreadManager = new GLThreadManager();
}
