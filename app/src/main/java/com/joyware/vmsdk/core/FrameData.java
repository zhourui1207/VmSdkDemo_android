package com.joyware.vmsdk.core;

/**
 * Created by zhourui on 16/11/10.
 * 一帧数据，接口
 */

public interface FrameData {
    int FRAME_TYPE_YUV = 0;
    int FRAME_TYPE_RGB = 1;

    int getFrameType();

    int getWidth();

    int getHeight();
}
