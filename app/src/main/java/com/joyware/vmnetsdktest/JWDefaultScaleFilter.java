package com.joyware.vmnetsdktest;

import android.content.Context;
import android.graphics.Matrix;
import android.graphics.PointF;
import android.util.Log;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.View;

/**
 * Created by zhourui on 17/4/30.
 * email: 332867864@qq.com
 * phone: 17006421542
 * 默认的缩放过滤器
 */

public class JWDefaultScaleFilter {

    private static final String TAG = "JWDefaultScaleFilter";

    private View mView;

    private float mLastTimeScaleFactor;
    private float mScaleCenterX;
    private float mScaleCenterY;
    private float mScalesRrcLeft;
    private float mScalesRrcTop;
    private float mScaleFactor = 1f;
    private float mLeftPercent;
    private float mTopPercent;
    private float mSrcLeft;
    private float mSrcTop;
    private boolean mDrag = false;
    private boolean mZoom = false;

    private float mMaxScaleFactor = 4f;
    private boolean mMustFillScreen = true;

    private ScaleGestureDetector mScaleGestureDetector;

    private Runnable mRunnable;

    private JWDefaultScaleFilter() {

    }

    public JWDefaultScaleFilter(Context context, Runnable runnable) {
        if (context == null) {
            throw new NullPointerException("context can not be null!");
        }
        mRunnable = runnable;
        mScaleGestureDetector = new ScaleGestureDetector(context, new ScaleGestureDetector.OnScaleGestureListener() {

            @Override
            public boolean onScale(ScaleGestureDetector detector) {
                if (mZoom) {
                    float sacleFactor = detector.getScaleFactor();
                    mScaleFactor *= sacleFactor;
                    if (mScaleFactor > mMaxScaleFactor) {
                        mScaleFactor = mMaxScaleFactor;
                    } else if (mScaleFactor < 1) {
                        mScaleFactor = 1;
                    }
                    float left = mScalesRrcLeft;
                    float top = mScalesRrcTop;

                    PointF pointF = scaleByPoint(left, top, mScaleCenterX,
                            mScaleCenterY, mScaleFactor / mLastTimeScaleFactor);

                    mLeftPercent = pointF.x / mView.getWidth();
                    mTopPercent = pointF.y / mView.getHeight();

                    defaultPointFilter();
                    if (mRunnable != null) {
                        mRunnable.run();
                    }
                }
                return true;
            }

            @Override
            public boolean onScaleBegin(ScaleGestureDetector detector) {
                if (!mZoom) {
                    mDrag = false;
                    mZoom = true;

                    // 左上角当前实际坐标 像素
                    mScalesRrcLeft = mLeftPercent * mView.getWidth();
                    mScalesRrcTop = mTopPercent * mView.getHeight();
                    mLastTimeScaleFactor = mScaleFactor;

                    mScaleCenterX = detector.getFocusX();
                    mScaleCenterY = detector.getFocusY();
                }

                // 中点坐标
                return true;
            }

            @Override
            public void onScaleEnd(ScaleGestureDetector detector) {

            }
        });
    }

    public void setView(View view) {
        mView = view;
    }

    public void setMaxScaleFactor(float maxScaleFactor) {
        mMaxScaleFactor = maxScaleFactor;
    }

    public void setMustFillScreen(boolean mustFillScreen) {
        mMustFillScreen = mustFillScreen;
    }

    public float getMaxScaleFactor() {
        return mMaxScaleFactor;
    }

    public boolean isMustFillScreen() {
        return mMustFillScreen;
    }

    public void reset() {
        mScaleFactor = 1f;
        mLeftPercent = 0f;
        mTopPercent = 0f;
    }

    public void isChanged(MotionEvent event) {
        if (mView == null) {
            Log.e(TAG, "view must be set");
            return;
        }
        mScaleGestureDetector.onTouchEvent(event);
        if (event.getPointerCount() > 1) {
            mDrag = false;
            if (event.getAction() == MotionEvent.ACTION_CANCEL || event.getAction() ==
                    MotionEvent.ACTION_UP || event.getAction() == MotionEvent
                    .ACTION_POINTER_UP) {
                mZoom = false;
            }
            return;
        }
        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN:
                mDrag = true;
                mSrcLeft = event.getRawX();
                mSrcTop = event.getRawY();
                break;
            case MotionEvent.ACTION_MOVE:
                if (mDrag) {
                    float currentX = event.getRawX();
                    float currentY = event.getRawY();
                    float offsetX = (currentX - mSrcLeft) / mView.getWidth();
                    float offsetY = (currentY - mSrcTop) / mView.getHeight();
                    mSrcLeft = currentX;
                    mSrcTop = currentY;
                    mLeftPercent += offsetX;
                    mTopPercent += offsetY;
                    defaultPointFilter();
                    if (mRunnable != null) {
                        mRunnable.run();
                    }
                }
                break;
            case MotionEvent.ACTION_CANCEL:
            case MotionEvent.ACTION_UP:
                mDrag = false;
                mZoom = false;
                break;
        }
    }

    public float getScaleFactor() {
        return mScaleFactor;
    }

    public float getLeftPercent() {
        return mLeftPercent;
    }

    public float getTopPercent() {
        return mTopPercent;
    }

    /**
     * 一个坐标点，以某个点为缩放中心，缩放指定倍数，求这个坐标点在缩放后的新坐标值。
     *
     * @param targetPointX 坐标点的X
     * @param targetPointY 坐标点的Y
     * @param scaleCenterX 缩放中心的X
     * @param scaleCenterY 缩放中心的Y
     * @param scale        缩放倍数
     * @return 坐标点的新坐标
     */
    protected PointF scaleByPoint(float targetPointX, float targetPointY, float scaleCenterX,
                                  float scaleCenterY, float scale) {
        Matrix matrix = new Matrix();
        // 将Matrix移到到当前圆所在的位置，
        // 然后再以某个点为中心进行缩放
        matrix.preTranslate(targetPointX, targetPointY);
        matrix.postScale(scale, scale, scaleCenterX, scaleCenterY);
        float[] values = new float[9];
        matrix.getValues(values);
        return new PointF(values[Matrix.MTRANS_X], values[Matrix.MTRANS_Y]);
    }

    // 点过滤器，过滤掉不符合规范的坐标
    private void defaultPointFilter() {
        // 范围，放大倍速不超过4.0，缩小倍速不能小于1.0，不能留出黑边
        // 黑边过滤
        if (mMustFillScreen) {
            if (mLeftPercent > 0f) {
                mLeftPercent = 0f;
            }
            if (mLeftPercent + mScaleFactor < 1f) {
                mLeftPercent = 1f - mScaleFactor;
            }
            if (mTopPercent > 0f) {
                mTopPercent = 0f;
            }
            if (mTopPercent + mScaleFactor < 1f) {
                mTopPercent = 1f - mScaleFactor;
            }
        }
    }
}
