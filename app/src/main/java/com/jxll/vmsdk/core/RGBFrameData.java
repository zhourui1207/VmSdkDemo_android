package com.jxll.vmsdk.core;

/**
 * Created by zhourui on 16/11/10.
 */

public class RGBFrameData implements FrameData {
  private byte[] data;
  private int width;
  private int height;
  @Override
  public int getFrameType() {
    return FRAME_TYPE_RGB;
  }

  @Override
  public int getWidth() {
    return width;
  }

  @Override
  public int getHeight() {
    return height;
  }

  private RGBFrameData() {

  }

  public RGBFrameData(byte[] data, int width, int height) {
    this.data = new byte[data.length];
    System.arraycopy(data, 0, this.data, 0, data.length);
    this.width = width;
    this.height = height;
  }

  public byte[] getData() {
    return data;
  }

}
