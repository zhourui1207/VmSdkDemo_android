package com.jxll.vmsdk.core;

/**
 * Created by zhourui on 16/11/9.
 */

public class FrameConfHolder {
    private int width;
    private int height;
    private int framerate;  // 帧率

    private void init(int width, int height, int framerate) {
        this.width = width;
        this.height = height;
        this.framerate = framerate;
    }

    public int getWidth() {
        return width;
    }

    public int getHeight() {
        return height;
    }

    public int getFramerate() {
        return framerate;
    }
}
