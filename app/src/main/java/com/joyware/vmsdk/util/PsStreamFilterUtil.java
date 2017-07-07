package com.joyware.vmsdk.util;

import android.util.Log;

/**
 * Created by zhourui on 17/2/9.
 * ps流工具
 */

public class PsStreamFilterUtil {

    private final String TAG = "PsStreamFilterUtil";

    public static final int STREAM_TYPE_MPEG4 = 0x10;
    public static final int STREAM_TYPE_H264 = 0x1B;
    public static final int STREAM_TYPE_SVAC_VIDEO = 0x80;
    public static final int STREAM_TYPE_G711 = 0x90;
    public static final int STREAM_TYPE_G722_1 = 0x92;
    public static final int STREAM_TYPE_G723_1 = 0x92;
    public static final int STREAM_TYPE_G729 = 0x99;
    public static final int STREAM_TYPE_SVAC_AUDIO = 0x9B;

    private boolean isDebug = false;

    private int trans = 0xFFFFFFFF;

    private boolean isVideo = true;

    // 找到数据起始
    private boolean haveData;

    private long pts;

    public interface OnDataCallback {
        void onESData(boolean video, long pts, byte[] outData, int outStart, int outLen);

        void onRemainingData(byte[] outData, int outStart, int outLen, int readedStart);
    }

    /**
     * 去除PS头
     * 返回新的begin
     */
    public void filterPsHeader(final byte[] data, int begin, int len, int readBegin,
                               OnDataCallback callback) {
        if (readBegin + len > data.length || len < 4 || callback == null) {
            return;
        }

        filter(data, begin, len, readBegin, callback);
    }

    private void filter(byte[] data, int begin, int len, int readBegin, OnDataCallback callback) {
        int tmp;

        // 当前读取字节位置
        int currentPosition = readBegin;

        // 数据开始位置
        int dataBegin = begin;

        try {
            // 循环遍历读取字节
            for (; currentPosition < readBegin + len; ++currentPosition) {
                tmp = (data[currentPosition]) & 0xFF;  // 转化成无符号

                //Log.e(TAG, "trans=" + trans);
                trans <<= 8;
                //Log.e(TAG, "trans=" + trans);
                trans |= tmp;

                // 判断读取到的4个字节
                if (trans == 0x000001BA) {  // ps头
                    trans = 0xFFFFFFFF;  // 先恢复这个判断变量

                    if (isDebug) {
                        Log.e(TAG, "ps头");
                    }

                    // 如果前面有数据，那么就调用回调让上层拼帧
                    if (haveData) {
                        if (callback != null) {
                            int outLen = currentPosition - 3 - dataBegin;
                            if (outLen > 0) {
                                callback.onESData(isVideo, pts, data, dataBegin, outLen);
                            }
                        }
                        haveData = false;
                    }

                    // i向后移动10字节
                    currentPosition += 10;

                    // 获取PS包头长度
                    if (currentPosition >= readBegin + len) {  // 长度不足，则全部过滤掉
                        return;
                    }

                    // 获取PS包头长度
                    int stuffingLength = data[currentPosition] & 0x07;

                    if (stuffingLength > 0) {  // 如果还有额外数据
                        currentPosition += stuffingLength;
                        if (currentPosition >= readBegin + len) {  // 长度不足，则全部过滤掉
                            return;
                        }
                    }

                    dataBegin = currentPosition + 1;
                } else if (trans == 0x000001BB || trans == 0x000001BC) {  // 系统标题头或节目映射流
                    trans = 0xFFFFFFFF;  // 先恢复这个判断变量

                    if (isDebug) {
                        Log.e(TAG, "系统标题头或节目映射流");
                    }

                    // 如果前面有数据，那么就调用回调让上层拼帧
                    if (haveData) {
                        if (callback != null) {
                            int outLen = currentPosition - 3 - dataBegin;
                            if (outLen > 0) {
                                callback.onESData(isVideo, pts, data, dataBegin, outLen);
                            }
                        }
                        haveData = false;
                    }

                    // i向后移动2字节
                    currentPosition += 2;

                    // 获取PS包头长度
                    if (currentPosition >= readBegin + len) {  // 长度不足，则全部过滤掉
                        return;
                    }

                    // 获取PS包头长度
                    byte high = data[currentPosition - 1];
                    byte low = data[currentPosition];
                    int length = high * 16 + low;

                    if (length > 0) {  // 如果还有额外数据
                        currentPosition += length;
                        if (currentPosition >= readBegin + len) {  // 长度不足，则全部过滤掉
                            return;
                        }
                    }

                    dataBegin = currentPosition + 1;
                } else if (((trans & 0xFFFFFFE0) == 0x000001E0) || ((trans & 0xFFFFFFE0) ==
                        0x000001C0)) {  // 视频头

                    // 如果前面有数据，那么就调用回调让上层拼帧
                    if (haveData) {
                        if (callback != null) {
                            int outLen = currentPosition - 3 - dataBegin;
                            if (outLen > 0) {
                                callback.onESData(isVideo, pts, data, dataBegin, outLen);
                            }
                        }
                    }

                    isVideo = (trans & 0xFFFFFFE0) == 0x000001E0;
                    trans = 0xFFFFFFFF;

                    if (isVideo) {
                        if (isDebug) {
                            Log.e(TAG, "视频头");
                        }
                    } else {
                        if (isDebug) {
                            Log.e(TAG, "音频头");
                        }
                    }
                    haveData = true;


                    // 向后移动4个字节判断是否有pts
                    currentPosition += 4;
                    if (currentPosition >= readBegin + len) {
                        return;
                    }

                    boolean havePts = false;
                    int ptsDpsMark = (data[currentPosition] & 0xc0) >> 7;
                    if (ptsDpsMark == 1) {
                        havePts = true;
                    }

                    // i向后移动1字节, pes包头长度
                    currentPosition += 1;
                    if (currentPosition >= readBegin + len) {  // 长度不足，则全部过滤掉
                        return;
                    }

                    // 获取PS包头长度
                    int length = data[currentPosition];

                    // 如果有pts，那么在pes包头长度后面的5各字节里面找
                    if (havePts) {
                        int ptsPosition = currentPosition + 5;
                        if (ptsPosition >= readBegin + len) {
                            return;
                        }
                        this.pts = ((data[currentPosition + 1] & 0x0e) << 29);

                        long tmpPts = ((data[currentPosition + 2] & 0xFF) << 8);
                        tmpPts += (data[currentPosition + 3] & 0xFF);
                        this.pts += ((tmpPts & 0xfffe) << 14);

                        tmpPts = ((data[currentPosition + 4] & 0xFF) << 8);
                        tmpPts += (data[currentPosition + 5] & 0xFF);
                        this.pts += ((tmpPts & 0xfffe) >> 1);
                    } else {
//                        this.pts = 0; 没有pts的话就默认用上前一个
                    }

                    if (length > 0) {  // 如果还有额外数据
                        currentPosition += length;
                        if (currentPosition >= readBegin + len) {  // 长度不足，则全部过滤掉
                            return;
                        }
                    }

                    // 将新起始点移动到i的后一位
                    dataBegin = currentPosition + 1;
                } else if (trans == 0x00000001) {  // 264头，有种情况是I帧的sps、pps、Iframe中夹杂着视频头
                    if (isDebug) {
                        Log.e(TAG, "发现264头");
                    }
//                    trans = 0xFFFFFFFF;
                    if (!haveData && (currentPosition - 3 > 0)) {
                        dataBegin = currentPosition - 3;
                        haveData = true;
                    }

                    isVideo = true;
                }
            }

        } finally {
            // 读完还有剩余数据，回调给上层供下次使用
            if (dataBegin < readBegin + len) {
                if (callback != null) {
                    int outLen = readBegin + len - dataBegin;
                    if (outLen > 0) {
                        callback.onRemainingData(data, dataBegin, outLen, currentPosition);
                    }
                }
            }
        }

    }
}
