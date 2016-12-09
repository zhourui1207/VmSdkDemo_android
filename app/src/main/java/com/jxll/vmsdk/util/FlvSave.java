package com.jxll.vmsdk.util;

/**
 * Created by zhourui on 16/10/31.
 */

import android.util.Log;

import java.io.FileOutputStream;
import java.io.IOException;


public class FlvSave {
  private static final String TAG = "FlvSave";

  private FileOutputStream mOut = null;
  private int mPreviousTagSize = 0;  //前一个TAG的长度
  private long mTimestamp = 0;

  private FlvSave() {

  }

  public FlvSave(FileOutputStream file) {
    mOut = file;
  }

  // 不创建路径，只负责创建文件
  public final boolean writeStart() {
    if (mOut == null) {
      return false;
    }

    try{
      // 现在只做 只有视频流头
      byte[] flvHeader = {0x46,0x4c,0x56,0x01,0x01,0x00,0x00,0x00,0x09};
      byte[] previousTagSize = {0x00, 0x00, 0x00, 0x00};
      mOut.write(flvHeader);
      mOut.write(previousTagSize);
    } catch (Exception e) {
      Log.e(TAG, StringUtil.getStackTraceAsString(e));
      return false;
    }
    return true;
  }

  //不包含 0x00 0x00 0x00 0x01
  public final boolean writeConfiguretion(byte[] sps, int spsstart, int spsSize, byte[] pps, int ppsstart, int ppsSize, long timestamp) {
    if (mOut == null) {
      return false;
    }

    byte[] videoHeader = {0x17, 0x00, 0x00, 0x00, 0x00};
    byte[] spsBuffer = new byte[spsSize];
    System.arraycopy(sps, spsstart, spsBuffer, 0, spsSize);
    byte[] ppsBuffer = new byte[ppsSize];
    System.arraycopy(pps, ppsstart, ppsBuffer, 0, ppsSize);
    byte[] avcConfiguretionHeader = {0x01, sps[1], sps[2], sps[3], (byte)0xff};
    byte[] spsBufferSize = {(byte)0xe1, 0x00, 0x00};
    byte[] ppsBufferSize = {0x01, 0x00, 0x00,};
    setInt(spsSize, spsBufferSize, 1, 2);
    setInt(ppsSize, ppsBufferSize, 1, 2);

    int dateSize = 5 + 5 + 3 + spsSize + 3 + ppsSize;
    byte[] tagHeader = {0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    setInt(dateSize, tagHeader, 1, 3);
    mPreviousTagSize = 11 + dateSize;

    byte[] previousTagSize = {0x00, 0x00, 0x00, 0x00};
    setInt(mPreviousTagSize, previousTagSize, 0, 4);

    try {
      mOut.write(tagHeader);
      mOut.write(videoHeader);
      mOut.write(avcConfiguretionHeader);
      mOut.write(spsBufferSize);
      mOut.write(spsBuffer);
      mOut.write(ppsBufferSize);
      mOut.write(ppsBuffer);
      mOut.write(previousTagSize);
    } catch (IOException e) {
      Log.e(TAG, StringUtil.getStackTraceAsString(e));
      return false;
    }
    mTimestamp = timestamp;
    if (mTimestamp == 0) {
      mTimestamp = System.currentTimeMillis();
    }
    return true;
  }

  //不包含 0x00 0x00 0x00 0x01
  public final boolean writeNalu(boolean isIFrame, byte[] buffer, int start, int size, long timestamp) {
    if (mOut == null) {
      return false;
    }

    byte[] videoConf = {0x27, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    if (isIFrame) {
      videoConf = new byte[]{0x17, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    }
    setInt(size, videoConf, 5, 4);
    byte[] videoData = new byte[size];
    System.arraycopy(buffer, start, videoData, 0, size);

    int dateSize = 9 + size;
    byte[] tagHeader = {0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    setInt(dateSize, tagHeader, 1, 3);
    if (timestamp == 0) {
      timestamp = System.currentTimeMillis();
    }
    setLong(((timestamp-mTimestamp) < 0 ? 0 : (timestamp-mTimestamp)), tagHeader, 4, 3);
    mPreviousTagSize = 11 + dateSize;

    byte[] previousTagSize = {0x00, 0x00, 0x00, 0x00};
    setInt(mPreviousTagSize, previousTagSize, 0, 4);

    try {
      mOut.write(tagHeader);
      mOut.write(videoConf);
      mOut.write(videoData);
      mOut.write(previousTagSize);
    } catch (IOException e) {
      Log.e(TAG, StringUtil.getStackTraceAsString(e));
      return false;
    }
    return true;
  }

  public void save() {
    if (mOut != null) {
      try {
        mOut.close();
      } catch (IOException e) {
        Log.e(TAG, StringUtil.getStackTraceAsString(e));
      }
    }
  }

  /** Sets long value */
  private static final void setLong(long n, byte[] data, int begin, int size) {
    int end = begin + size;
    for (end--; end >= begin; end--) {
      data[end] = (byte) (n % 256);
      n >>= 8;
    }
  }

  /** Sets Int value */
  private static final void setInt(int n, byte[] data, int begin, int size) {
    setLong(n, data, begin, size);
  }
}
