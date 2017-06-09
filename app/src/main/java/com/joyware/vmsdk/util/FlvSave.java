package com.joyware.vmsdk.util;

/**
 * Created by zhourui on 16/10/31.
 */

import android.util.Log;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.RandomAccessFile;


public class FlvSave {
    private static final String TAG = "FlvSave";

    private FileOutputStream mOut = null;
    private int mPreviousTagSize = 0;  //前一个TAG的长度
    private long mTimestamp = 0;
    private long mLastTimetamp = 0;

    private long mAudioTimestamp = 0;

    private String mFileName;

    private boolean mHasMateData = false;

    private FlvSave() {

    }

    public FlvSave(String fileName) throws FileNotFoundException {
        mFileName = fileName;
        mOut = new FileOutputStream(fileName);
    }

    /**
     * Sets long value
     */
    private static final void setLong(long n, byte[] data, int begin, int size) {
        int end = begin + size;
        for (end--; end >= begin; end--) {
            data[end] = (byte) (n % 256);
            n >>= 8;
        }
    }

    /**
     * Sets Int value
     */
    private static final void setInt(int n, byte[] data, int begin, int size) {
        setLong(n, data, begin, size);
    }

    // 不创建路径，只负责创建文件
    public final boolean writeStart(boolean hasMateData) {
        if (mOut == null) {
            return false;
        }

        mHasMateData = hasMateData;

        try {
            // 现在只做 只有视频流头
            byte[] flvHeader = {0x46, 0x4c, 0x56, 0x01, 0x01, 0x00, 0x00, 0x00, 0x09};
            // 既有视频，又有音频
//            byte[] flvHeader = {0x46, 0x4c, 0x56, 0x01, 0x05, 0x00, 0x00, 0x00, 0x09};
            byte[] previousTagSize = {0x00, 0x00, 0x00, 0x00};

            mOut.write(flvHeader);
            mOut.write(previousTagSize);

            if (mHasMateData) {
                byte[] scriptTag = new byte[55];
                mOut.write(scriptTag);
            }
        } catch (Exception e) {
            Log.e(TAG, StringUtil.getStackTraceAsString(e));
            return false;
        }
        return true;
    }

    //不包含 0x00 0x00 0x00 0x01
    public final boolean writeConfiguretion(byte[] sps, int spsstart, int spsSize, byte[] pps,
                                            int ppsstart, int ppsSize) {
        if (mOut == null) {
            return false;
        }

        byte[] videoHeader = {0x17, 0x00, 0x00, 0x00, 0x00};
        byte[] spsBuffer = new byte[spsSize];
        System.arraycopy(sps, spsstart, spsBuffer, 0, spsSize);
        byte[] ppsBuffer = new byte[ppsSize];
        System.arraycopy(pps, ppsstart, ppsBuffer, 0, ppsSize);
        byte[] avcConfiguretionHeader = {0x01, sps[1], sps[2], sps[3], (byte) 0xff};
        byte[] spsBufferSize = {(byte) 0xe1, 0x00, 0x00};
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
        mTimestamp = System.currentTimeMillis();
        mAudioTimestamp = System.currentTimeMillis();
//        mTimestamp = timestamp;
//        if (mTimestamp == 0) {
//            mTimestamp = System.currentTimeMillis();
//        }
        return true;
    }

    //不包含 0x00 0x00 0x00 0x01
    public final boolean writeNalu(boolean isIFrame, byte[] buffer, int start, int size) {
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

        int dateSize = videoConf.length + size;
        byte[] tagHeader = {0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        setInt(dateSize, tagHeader, 1, 3);

        long timestamp = System.currentTimeMillis();
//        if (timestamp == 0) {
//            timestamp = System.currentTimeMillis();
//        }
        mLastTimetamp = ((timestamp - mTimestamp) < 0 ? 0 : (timestamp - mTimestamp));
        setLong(mLastTimetamp, tagHeader, 4, 3);
        mPreviousTagSize = tagHeader.length + dateSize;

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

    public final boolean writeAudioPcm(byte[] buffer, int start, int size, long timestamp) {
        if (mOut == null) {
            return false;
        }

        Log.e(TAG, "size=" + size);
//
//        // 8000->5500  80->55  16->11
        int newLen = size * 11 / 16;
        if (newLen % 2 != 0) {
            newLen += 1;
        }
        byte[] tmpBuffer = new byte[newLen];
//        int tmpIndex = 0;
//
//        int missCount = (size - newLen) / 2;  // 双字节一个数据
//
//        int missRate = newLen / (missCount + 1);
//
//        if (missRate % 2 != 0) {
//            missRate += 1;
//        }
//
//        int startLength = missRate / 2;
//        int endLength = startLength;
//        if (startLength % 2 != 0) {
//            startLength += 1;
//            endLength -= 1;
//        }
//
//        System.arraycopy(buffer, 0, tmpBuffer, tmpIndex, startLength);
//        tmpIndex += startLength;
//
//        for (int i = startLength; i <= start + size; i += (missRate+1)) {
//            System.arraycopy(buffer, i + 1, tmpBuffer, tmpIndex, missRate);
//            tmpIndex += missRate;
//        }
//
//        System.arraycopy(buffer, start + size - (endLength + 1), tmpBuffer, tmpIndex, endLength);

        System.arraycopy(buffer, 0, tmpBuffer, 0, newLen);

//        byte[] audioConf = {0x7e, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        byte[] audioConf = {0x02};
//        setInt(size, audioConf, 5, 4);
//        byte[] audioData = new byte[size];
//        System.arraycopy(buffer, start, audioData, 0, size);

//        int dateSize = audioConf.length + size;
        int dateSize = audioConf.length + newLen;
        byte[] tagHeader = {0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        setInt(dateSize, tagHeader, 1, 3);

        timestamp = System.currentTimeMillis();
        mLastTimetamp = ((timestamp - mTimestamp) < 0 ? 0 : (timestamp - mTimestamp));
        setLong(mLastTimetamp, tagHeader, 4, 3);

        mPreviousTagSize = tagHeader.length + dateSize;

        byte[] previousTagSize = {0x00, 0x00, 0x00, 0x00};
        setInt(mPreviousTagSize, previousTagSize, 0, 4);

        try {
            mOut.write(tagHeader);
            mOut.write(audioConf);
            mOut.write(tmpBuffer);
            mOut.write(previousTagSize);
        } catch (IOException e) {
            Log.e(TAG, StringUtil.getStackTraceAsString(e));
            return false;
        }
        return true;
    }

    public final boolean writeAudioAAC(byte[] buffer, int start, int size, long timestamp) {
        if (mOut == null) {
            return false;
        }

//        byte[] audioConf = {0x7e, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        byte[] audioConf = {0x36};
//        setInt(size, audioConf, 5, 4);
        byte[] audioData = new byte[size];
        System.arraycopy(buffer, start, audioData, 0, size);

        int dateSize = audioConf.length + size;
        byte[] tagHeader = {0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        setInt(dateSize, tagHeader, 1, 3);

        timestamp = System.currentTimeMillis();
        mLastTimetamp = ((timestamp - mTimestamp) < 0 ? 0 : (timestamp - mTimestamp));
        setLong(mLastTimetamp, tagHeader, 4, 3);

        mPreviousTagSize = tagHeader.length + dateSize;

        byte[] previousTagSize = {0x00, 0x00, 0x00, 0x00};
        setInt(mPreviousTagSize, previousTagSize, 0, 4);

        try {
            mOut.write(tagHeader);
            mOut.write(audioConf);
            mOut.write(audioData);
            mOut.write(previousTagSize);
        } catch (IOException e) {
            Log.e(TAG, StringUtil.getStackTraceAsString(e));
            return false;
        }
        return true;
    }

    private void writeMetaData(RandomAccessFile randomAccessFile) {

//        setInt(size, audioConf, 5, 4);
        byte[] scriptData = {0x02, 0x00, 0x0a, 0x6f, 0x6e, 0x4d, 0x65, 0x74, 0x61, 0x44, 0x61,
                0x74, 0x61, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x08, 0x64, 0x75, 0x72, 0x61,
                0x74, 0x69, 0x6f, 0x6e, 0x00};

        byte[] durationData = double2Bytes(1D * mLastTimetamp / 1000);
//        Log.e(TAG, "timestamp=" + (1D * mLastTimetamp / 1000));
//        byte[] durationData = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
//        setLong(mLastTimetamp, durationData, 0, 8);

        byte[] scriptDataEnd = {0x00, 0x00, 0x09};

        int dataSize = scriptData.length + durationData.length + scriptDataEnd.length;

        byte[] tagHeader = {0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};  //
        setInt(dataSize, tagHeader, 1, 3);

        mPreviousTagSize = tagHeader.length + dataSize;

        byte[] previousTagSize = {0x00, 0x00, 0x00, 0x00};
        setInt(mPreviousTagSize, previousTagSize, 0, 4);

        try {
            randomAccessFile.write(tagHeader);
            randomAccessFile.write(scriptData);
            randomAccessFile.write(durationData);
            randomAccessFile.write(scriptDataEnd);
            randomAccessFile.write(previousTagSize);
        } catch (IOException e) {
            Log.e(TAG, StringUtil.getStackTraceAsString(e));
        }
    }

    public boolean save() {
        if (mOut != null) {
            try {
//                mOut.
//                writeMetaData();
                mOut.close();
                if (mHasMateData) {
                    RandomAccessFile randomAccessFile = new RandomAccessFile(mFileName, "rw");
                    randomAccessFile.seek(13);
                    writeMetaData(randomAccessFile);
                    randomAccessFile.close();
                }

            } catch (IOException e) {
                Log.e(TAG, StringUtil.getStackTraceAsString(e));
            }
            return true;
        }

        return false;
    }

    public void cancel() {
        if (mOut != null) {
            try {
//                mOut.
//                writeMetaData();
                mOut.close();
            } catch (IOException e) {
                Log.e(TAG, StringUtil.getStackTraceAsString(e));
            }

            File file = new File(mFileName);
            file.delete();
        }
    }

    private static byte[] double2Bytes(double d) {
        long value = Double.doubleToRawLongBits(d);
        byte[] byteRet = new byte[8];
        for (int i = 0; i < 8; i++) {
            byteRet[7-i] = (byte) ((value >> 8 * i) & 0xff);
        }
        return byteRet;
    }

    private static double bytes2Double(byte[] arr) {
        long value = 0;
        for (int i = 0; i < 8; i++) {
            value |= ((long) (arr[i] & 0xff)) << (8 * i);
        }
        return Double.longBitsToDouble(value);
    }

}
