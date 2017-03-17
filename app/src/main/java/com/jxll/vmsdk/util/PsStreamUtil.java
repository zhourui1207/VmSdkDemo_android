package com.jxll.vmsdk.util;

import android.util.Log;

/**
 * Created by zhourui on 17/2/9.
 * ps流工具
 */

public class PsStreamUtil {

    private final String TAG = "PsStreamUtil";

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

    private boolean isPreVideo = true;

    // 找到数据起始
    private boolean findDataStart = false;

    // 是否需要再次输入,解决音视频连在一个包中的情况
    private boolean needAgain = true;

    public boolean isVideo() {
        return isVideo;
    }

    public boolean isFindDataStart() {
        return findDataStart;
    }

    public boolean isNeedAgain() {
        return needAgain;
    }

    /**
     * 去除PS头
     * 返回新的begin
     */
    public int filterPsHeader(byte[] data, int begin, int len) {
        if (begin + len > data.length || len < 4) {
            return len;
        }

        isVideo = isPreVideo;

//        Log.e(TAG, "time start=" + System.currentTimeMillis());
        int beginNew = filter(data, begin, len);
//        Log.e(TAG, "time end=" + System.currentTimeMillis());
        return beginNew;
    }

    private int filter(byte[] data, int begin, int len) {
        int beginNew = begin;  // 新的起始点，若没有匹配，则为begin
        int tmp;

        // 临时视频数据
//        byte[] tmpData = new byte[len];  // 不会超过len;
        int tmpLen = 0;

//        Log.e(TAG, "输入长度 len=" + len + ", begin=" + begin);
        for (int i = 0; i < len; ++i) {
            tmp = (data[i + begin]) & 0xFF;  // 转化成无符号

            // 如果临时数据不为空，那么就是视频数据，需要保存了
//            Log.e(TAG, "i=" + i);
//            tmpData[tmpLen++] = data[i + begin];
            tmpLen++;

            //Log.e(TAG, "trans=" + trans);
            trans <<= 8;
            //Log.e(TAG, "trans=" + trans);
            trans |= tmp;

            //Log.e(TAG, "tmp=" + tmp + ", trans=" + trans + ", 0x000001BA=" + 0x000001BA + ",0xFFFFFFFF=" + 0xFFFFFFFF);

            if (trans == 0x000001BA) {  // ps头
                trans = 0xFFFFFFFF;  // 先恢复这个判断变量

                if (tmpLen >= 4) {
                    tmpLen -= 4;  // 向前移动4位
                    // 将新起始点移动到i的后一位
                }

                if (isDebug) {
                    Log.e(TAG, "ps头");
                }

                // i向后移动10字节
                i += 10;

                // 获取PS包头长度
                if (i >= len) {  // 长度不足，则全部过滤掉
                    beginNew = len;
                    return beginNew;
                }

                // 获取PS包头长度
                int stuffingLength = data[begin + i] & 0x07;

                if (stuffingLength > 0) {  // 如果还有额外数据
                    i += stuffingLength;
                    if (i >= len) {  // 长度不足，则全部过滤掉
                        beginNew = len;
                        return beginNew;
                    }
                }

                // 将新起始点移动到i的后一位
                beginNew += (i + 1);

                break;
            } else if (trans == 0x000001BB || trans == 0x000001BC) {  // 系统标题头或节目映射流
                trans = 0xFFFFFFFF;  // 先恢复这个判断变量

                if (tmpLen >= 4) {
                    tmpLen -= 4;  // 向前移动4位
                }

                if (isDebug) {
                    Log.e(TAG, "系统标题头或节目映射流");
                }

                // i向后移动2字节
                i += 2;

                // 获取PS包头长度
                if (i >= len) {  // 长度不足，则全部过滤掉
                    beginNew = len;
                    return beginNew;
                }

                // 获取PS包头长度
                byte high = data[i -1 + begin];
                byte low = data[i + begin];
                int length = high * 16 + low;

                if (length > 0) {  // 如果还有额外数据
                    i += length;
                    if (i >= len) {  // 长度不足，则全部过滤掉
                        beginNew = len;
                        return beginNew;
                    }
                }

                // 将新起始点移动到i的后一位
                beginNew += (i + 1);

                break;
            } else if ((trans & 0xFFFFFFE0) == 0x000001E0) {  // 视频头
                trans = 0xFFFFFFFF;
                findDataStart = true;

                if (tmpLen >= 4) {
                    tmpLen -= 4;  // 向前移动4位
                }

                if (tmpLen > 0 && !isVideo) {
                    isPreVideo = true;
                } else {
                    isVideo = true;
                }

                if (isDebug) {
                    Log.e(TAG, "视频头");
                }

                // i向后移动5字节
                i += 5;

                // 获取PS包头长度
                if (i >= len) {  // 长度不足，则全部过滤掉
                    beginNew = len;
                    return beginNew;
                }

                // 获取PS包头长度
                int length = data[begin + i];

                if (length > 0) {  // 如果还有额外数据
                    i += length;
                    if (i >= len) {  // 长度不足，则全部过滤掉
                        beginNew = len;
                        return beginNew;
                    }
                }

                // 将新起始点移动到i的后一位
                beginNew += (i + 1);

                break;
            } else if ((trans & 0xFFFFFFE0) == 0x000001C0) {  // 音频头
                trans = 0xFFFFFFFF;
                findDataStart = true;

                if (tmpLen >= 4) {
                    tmpLen -= 4;  // 向前移动4位
                }

                if (tmpLen > 0 && isVideo) {
                    isPreVideo = false;
                } else {
                    isVideo = false;
                }

                if (isDebug) {
                    Log.e(TAG, "音频头");
                }

                // i向后移动5字节
                i += 5;

                // 获取PS包头长度
                if (i >= len) {  // 长度不足，则全部过滤掉
                    beginNew = len;
                    return beginNew;
                }

                // 获取PS包头长度
                int length = data[begin + i];

                if (length > 0) {  // 如果还有额外数据
                    i += length;
                    if (i >= len) {  // 长度不足，则全部过滤掉
                        beginNew = len;
                        return beginNew;
                    }
                }

                // 将新起始点移动到i的后一位
                beginNew += (i + 1);

                break;
            } else if (trans == 0x00000001) {  // 264头，有种情况是I帧的sps、pps、Iframe中夹杂着视频头
                if (isDebug) {
                    Log.e(TAG, "发现264头");
                }
                trans = 0xFFFFFFFF;
                findDataStart = true;
                isVideo = true;

                // 加上分隔符

//                tmpData[tmpBegin++] = 0x00;
//                tmpData[tmpBegin++] = 0x00;
//                tmpData[tmpBegin++] = 0x00;
//                tmpData[tmpBegin++] = 0x01;

//                break;
            }
        }

        // 如果还有未过滤完的数据，则继续过滤
        if (beginNew > begin && beginNew < begin + len) {
            beginNew = filter(data, beginNew, len - (beginNew - begin));
        }

//        Log.e(TAG, "beginNew=" + beginNew);
        // 有视频数据在，那么再加到前面去
        if (tmpLen > 0 && beginNew > begin) {
//            Log.e(TAG, "复制视频数据 起点" +(beginNew - tmpBegin) + " 长度" + tmpBegin);
            byte[] tmpData = new byte[tmpLen];
            try {
                System.arraycopy(data, begin, tmpData, 0, tmpLen);
                System.arraycopy(tmpData, 0, data, beginNew - tmpLen, tmpLen);
            } catch (Exception e) {
                Log.e(TAG, "数据拷贝异常:" + e.getMessage());
            }
        }

//        Log.e(TAG, "过滤后起始点" + (beginNew > begin ? beginNew - tmpBegin : beginNew));
        return beginNew > begin ? beginNew - tmpLen : beginNew;
    }
}
