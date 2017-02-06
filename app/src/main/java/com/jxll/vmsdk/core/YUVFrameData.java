package com.jxll.vmsdk.core;

/**
 * Created by zhourui on 16/11/10.
 */

public class YUVFrameData implements FrameData {
    private byte[] yData;
    private byte[] uData;
    private byte[] vData;
    private int width;
    private int height;

    private YUVFrameData() {

    }

    public YUVFrameData(byte[] yData, byte[] uData, byte[] vData, int width, int height) {
        this.yData = new byte[yData.length];
        System.arraycopy(yData, 0, this.yData, 0, yData.length);

        this.uData = new byte[uData.length];
        System.arraycopy(uData, 0, this.uData, 0, uData.length);

        this.vData = new byte[vData.length];
        System.arraycopy(vData, 0, this.vData, 0, vData.length);

        this.width = width;
        this.height = height;
    }

    @Override
    public int getFrameType() {
        return FRAME_TYPE_YUV;
    }

    @Override
    public int getWidth() {
        return width;
    }

    @Override
    public int getHeight() {
        return height;
    }

    public byte[] getyData() {
        return yData;
    }


    public byte[] getuData() {
        return uData;
    }


    public byte[] getvData() {
        return vData;
    }

}
