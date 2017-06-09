package com.joyware.vmsdk.core;

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

    public YUVFrameData(byte[] yData, int yStart, int yLen, byte[] uData, int uStart, int uLen,
                        byte[] vData, int vStart, int vLen, int width, int height) {
        this.yData = new byte[yLen];
        System.arraycopy(yData, yStart, this.yData, 0, yLen);

        this.uData = new byte[uLen];
        System.arraycopy(uData, uStart, this.uData, 0, uLen);

        this.vData = new byte[vLen];
        System.arraycopy(vData, vStart, this.vData, 0, vLen);

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
