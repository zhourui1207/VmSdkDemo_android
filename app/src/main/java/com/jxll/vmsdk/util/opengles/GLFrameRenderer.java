package com.jxll.vmsdk.util.opengles;

import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.util.Log;

import java.nio.ByteBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Created by zhourui on 16/11/3.
 */

public class GLFrameRenderer implements GLSurfaceView.Renderer {
  private String TAG = "GLFrameRenderer";

  private GLSurfaceView mTargetSurface;
  private GLProgram prog = new GLProgram(0);
  private int mVideoWidth = -1, mVideoHeight = -1;
  private ByteBuffer y;
  private ByteBuffer u;
  private ByteBuffer v;

  public GLFrameRenderer(GLSurfaceView surface) {
    mTargetSurface = surface;
  }

  @Override
  public void onSurfaceCreated(GL10 gl, EGLConfig config) {
    Log.e(TAG, "GLFrameRenderer :: onSurfaceCreated");
    if (!prog.isProgramBuilt()) {
      prog.buildProgram();
      Log.e(TAG, "GLFrameRenderer :: buildProgram done");
    }
  }

  @Override
  public void onSurfaceChanged(GL10 gl, int width, int height) {
    Log.e(TAG, "GLFrameRenderer :: onSurfaceChanged");
    GLES20.glViewport(0, 0, width, height);
  }

  @Override
  public void onDrawFrame(GL10 gl) {
    synchronized (this) {
      if (y != null) {
        // reset position, have to be done
        y.position(0);
        u.position(0);
        v.position(0);
        prog.buildTextures(y, u, v, mVideoWidth, mVideoHeight);
        GLES20.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
        prog.drawFrame();
      }
    }
  }

  /**
   * this method will be called from native code, it happens when the video is about to play or
   * the video size changes.
   */
  public void update(int w, int h) {
    Log.e(TAG, "INIT E w=" + w + "h=" + h);
    if (w > 0 && h > 0) {
      if (w != mVideoWidth && h != mVideoHeight) {
        this.mVideoWidth = w;
        this.mVideoHeight = h;
        int yarraySize = w * h;
        int uvarraySize = yarraySize / 4;
        synchronized (this) {
          y = ByteBuffer.allocate(yarraySize);
          u = ByteBuffer.allocate(uvarraySize);
          v = ByteBuffer.allocate(uvarraySize);
        }
      }
    }

    Log.e(TAG, "INIT X");
  }

  /**
   * this method will be called from native code, it's used for passing yuv data to me.
   */
  public void update(byte[] ydata, byte[] udata, byte[] vdata) {
    synchronized (this) {
      y.clear();
      u.clear();
      v.clear();
      y.put(ydata, 0, ydata.length);
      u.put(udata, 0, udata.length);
      v.put(vdata, 0, vdata.length);
    }

    // request to render
    mTargetSurface.requestRender();
  }
}