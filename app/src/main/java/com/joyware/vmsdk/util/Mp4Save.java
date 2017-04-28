package com.joyware.vmsdk.util;

/**
 * Created by zhourui on 16/10/31.
 */

import android.support.annotation.NonNull;
import android.util.Log;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.LinkedList;
import java.util.List;


public class Mp4Save {
    private static final String TAG = "Mp4Save";

    private FileOutputStream mOut = null;

//    private long mStartTimetamp = 0;
//    private long mLastTimetamp = 0;

    //    private long mPreAudioTimestamp = 0;
//    private long mLastAudioTimestamp = 0;

    private String mFileName;

    private int mFramerate;
    private long mAudioSample;

    private int mWidth = 0;
    private int mHeight = 0;

    private int mFrameCount = 0;  // 帧数量
    private int mAudioFrameCount = 0;  // 音频帧数量
    private int mAudioPreVideoFrameCount = 0;

    private byte[] mSpsBuffer;
    private byte[] mPpsBuffer;
    private byte[] mSeiBuffer;

    @NonNull
    private List<Integer> mVideoSampleSizes = new LinkedList<>();  // 视频样本大小

    @NonNull
    private List<Integer> mAudioSampleSizes = new LinkedList<>();  // 音频样本大小

    private long mCurrentIndex = 0;  // 当前文件写指针

    @NonNull
    private List<Long> mVideoSampleIndexs = new LinkedList<>();  // 视频样本相对文件起始字节数

    @NonNull
    private List<Long> mAudioSampleIndexs = new LinkedList<>();  // 视频样本相对文件起始字节数

//    private List<Long> mAudioSampleTimestamps = new LinkedList<>();  // 时间戳列表

    @NonNull
    private List<Integer> mVideoIFrames = new LinkedList<>();  // 关键帧索引

    private Mp4Save() {

    }

    public Mp4Save(String fileName) throws FileNotFoundException {
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
    public final boolean writeStart() {
        if (mOut == null) {
            return false;
        }

        try {
//            00 00 00 1C 66 74 79 70    6D 70 34 32 00 00 00 00    ....ftypmp......
//            69 73 6F 6D
            byte[] mp4Header = {0x00, 0x00, 0x00, 0x1c, 0x66, 0x74, 0x79, 0x70,
                    0x6d, 0x70, 0x34, 0x32, 0x00, 0x00, 0x00, 0x00,
                    0x6d, 0x70, 0x34, 0x32, 0x69, 0x73, 0x6f, 0x6d,
                    0x48, 0x4b, 0x4d, 0x49};

            mOut.write(mp4Header);

            mCurrentIndex += mp4Header.length;

        } catch (Exception e) {
            Log.e(TAG, StringUtil.getStackTraceAsString(e));
            return false;
        }
        return true;
    }

    //不包含 0x00 0x00 0x00 0x01
    public final boolean writeConfiguretion(int width, int height, int framerate, byte[] sps,
                                            int spsstart, int spsSize, byte[] pps,
                                            int ppsstart, int ppsSize) {
        if (mOut == null) {
            return false;
        }

        mWidth = width;
        mHeight = height;
        mFramerate = framerate;
        Log.w(TAG, "width=" + width + ", height=" + height + ", framerate=" + framerate);

        if (mSpsBuffer == null) {
            mSpsBuffer = new byte[spsSize];
            System.arraycopy(sps, spsstart, mSpsBuffer, 0, spsSize);
        }
        if (mPpsBuffer == null) {
            mPpsBuffer = new byte[ppsSize];
            System.arraycopy(pps, ppsstart, mPpsBuffer, 0, ppsSize);
        }

        return true;
    }

    //不包含 0x00 0x00 0x00 0x01
    public final boolean writeNalu(boolean isIFrame, byte[] buffer, int start, int size) {
        if (mOut == null) {
            return false;
        }

        if ((buffer[start] & 0x1f) == 6) {  // sei
            if (mSeiBuffer == null) {
                mSeiBuffer = new byte[size];
                System.arraycopy(buffer, start, mSeiBuffer, 0, size);
            }
            return true;
        }

        ++mFrameCount;
        // 视频样本大小，样本相对文件起始字节，关键帧序列
//        byte[] mdat = {0x00, 0x00, 0x00, 0x12, 0x6d, 0x64, 0x61, 0x74,
//                0x00, 0x00, 0x00, 0x02, 0x09, 0x30};

        byte[] mdat = {0x00, 0x00, 0x00, 0x12, 0x6d, 0x64, 0x61, 0x74};

        int sampleSize = mdat.length + 4 + size;

        if (isIFrame) {
//            mdat[13] = 0x10;
            if (mSpsBuffer != null) {
                sampleSize += (4 + mSpsBuffer.length);
            }
            if (mPpsBuffer != null) {
                sampleSize += (4 + mPpsBuffer.length);
            }
            if (mSeiBuffer != null) {
                sampleSize += (4 + mSeiBuffer.length);
            }
        }

        byte[] sizeBuffer = {0x00, 0x00, 0x00, 0x00};
        setInt(size, sizeBuffer, 0, 4);

        setInt(sampleSize, mdat, 0, 4);

//        byte[] sample = new byte[size + mdat.length];

//        System.arraycopy(mdat, 0, sample, 0, mdat.length);
//        System.arraycopy();

        mVideoSampleSizes.add(sampleSize - 8);
        mVideoSampleIndexs.add(mCurrentIndex + 8);
        if (isIFrame) {
            mVideoIFrames.add(mFrameCount);
        }

        mCurrentIndex += sampleSize;

//        if (mStartTimetamp == 0) {
//            mStartTimetamp = System.currentTimeMillis();
//            mLastTimetamp = 0;
//        } else {
//            mLastTimetamp = (System.currentTimeMillis() - mStartTimetamp) * 90;  //90000
//        }

        try {
            mOut.write(mdat);
            if (isIFrame) {
                if (mSpsBuffer != null) {
                    byte[] spsLenBuffer = {0x00, 0x00, 0x00, 0x00};
                    setInt(mSpsBuffer.length, spsLenBuffer, 0, 4);
                    mOut.write(spsLenBuffer);
                    mOut.write(mSpsBuffer);
                }

                if (mPpsBuffer != null) {
                    byte[] ppsLenBuffer = {0x00, 0x00, 0x00, 0x00};
                    setInt(mPpsBuffer.length, ppsLenBuffer, 0, 4);
                    mOut.write(ppsLenBuffer);
                    mOut.write(mPpsBuffer);
                }

                if (mSeiBuffer != null) {
                    byte[] seiLenBuffer = {0x00, 0x00, 0x00, 0x00};
                    setInt(mSeiBuffer.length, seiLenBuffer, 0, 4);
                    mOut.write(seiLenBuffer);
                    mOut.write(mSeiBuffer);
                }

            }
            mOut.write(sizeBuffer);
            mOut.write(buffer, start, size);
        } catch (IOException e) {
            Log.e(TAG, StringUtil.getStackTraceAsString(e));
            return false;
        }
        return true;
    }

    public final boolean writeAAC(byte[] buffer, int start, int size) {
        if (mOut == null) {
            return false;
        }

        ++mAudioFrameCount;
        mAudioPreVideoFrameCount = mFrameCount;

        byte[] mdat = {0x00, 0x00, 0x00, 0x12, 0x6d, 0x64, 0x61, 0x74};

        int sampleSize = mdat.length + size;

        setInt(sampleSize, mdat, 0, 4);

        mAudioSampleSizes.add(sampleSize - 8);
        mAudioSampleIndexs.add(mCurrentIndex + 8);

        mCurrentIndex += sampleSize;

//        if (mStartTimetamp == 0) {
//            mStartTimetamp = System.currentTimeMillis();
//            mLastAudioTimestamp = 0;
//        } else {
//            mLastAudioTimestamp = (System.currentTimeMillis() - mStartTimetamp) * 8;  // 8000采样率
//        }

//        mAudioSampleTimestamps.add(mLastAudioTimestamp - mPreAudioTimestamp);

//        mPreAudioTimestamp = mLastAudioTimestamp;

        try {
            mOut.write(mdat);
//            mOut.write(sizeBuffer);
            mOut.write(buffer, start, size);
        } catch (IOException e) {
            Log.e(TAG, StringUtil.getStackTraceAsString(e));
            return false;
        }

        return true;
    }

    public void save() {
        if (mOut != null) {
            try {
                byte[] tmpBuffer = new byte[10240 + mAudioSampleIndexs.size() * 16 +
                        mVideoSampleIndexs.size() * 16];
                int writeLen = writeMoov(tmpBuffer, 0);
                mOut.write(tmpBuffer, 0, writeLen);

                mOut.close();
            } catch (Exception e) {
                Log.e(TAG, StringUtil.getStackTraceAsString(e));
            }
        }
    }

    private static byte[] double2Bytes(double d) {
        long value = Double.doubleToRawLongBits(d);
        byte[] byteRet = new byte[8];
        for (int i = 0; i < 8; i++) {
            byteRet[7 - i] = (byte) ((value >> 8 * i) & 0xff);
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

    private int writeMoov(byte[] tmpBuffer, int begin) {
        byte[] moov = {0x00, 0x00, 0x00, 0x08, 0x6d, 0x6f, 0x6f, 0x76};

        int boxSize = moov.length;

        boxSize += writeMvhd(tmpBuffer, begin + boxSize);  // 写入mvhd
        boxSize += writeTrak(true, tmpBuffer, begin + boxSize);  // 写入video trak
        if (mAudioFrameCount > 0) {
            // 视频和音频总时间相同，计算每帧音频时间间隔
//            mAudioSample = (long) (1.0f * mAudioPreVideoFrameCount / mAudioFrameCount / mFramerate * 8000);
            mAudioSample = 1024;
            boxSize += writeTrak(false, tmpBuffer, begin + boxSize);  // 写入audio trak
        }

        setInt(boxSize, moov, 0, 4);  // 写入boxsize

        System.arraycopy(moov, 0, tmpBuffer, begin, moov.length);  // 写入boxtype

        return boxSize;
    }

    private int writeMvhd(byte[] tmpBuffer, int begin) {
        byte[] mvhd = {0x00, 0x00, 0x00, 0x6c, 0x6d, 0x76, 0x68, 0x64,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, (byte) 0xe8,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x5f, (byte) 0x90,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
                0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x02};

//        setLong(mLastTimetamp, mvhd, 24, 4);
        setLong((long) (1.0f / mFramerate * 90000 * mFrameCount), mvhd, 24, 4);

        if (mAudioFrameCount > 0) {
            mvhd[107] = 0x03;
        }

        System.arraycopy(mvhd, 0, tmpBuffer, begin, 108);

        return 108;
    }

    private int writeTrak(boolean isVideo, byte[] tmpBuffer, int begin) {
        // 音视频相同
        byte[] trak = {0x00, 0x00, 0x00, 0x08, 0x74, 0x72, 0x61, 0x6b};

        int boxSize = trak.length;

        boxSize += writeTkhd(isVideo, tmpBuffer, begin + boxSize);  // 写入mvhd
        boxSize += writeMdia(isVideo, tmpBuffer, begin + boxSize);  // 写入trak

        setInt(boxSize, trak, 0, 4);  // 写入boxsize

        System.arraycopy(trak, 0, tmpBuffer, begin, trak.length);  // 写入boxtype

        return boxSize;
    }

    private int writeTkhd(boolean isVideo, byte[] tmpBuffer, int begin) {
        byte[] tkhd = {0x00, 0x00, 0x00, 0x5c, 0x74, 0x6b, 0x68, 0x64,
                0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00,  // 后4个生成时间
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,  // 前4个修改时间
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 后4个是总时间
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 保留
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 最后两个保留,倒数3、4是音量
                0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // width
                0x00, 0x00, 0x00, 0x00};  // height

//        setLong(isVideo ? mLastTimetamp : mLastAudioTimestamp, tkhd, 28, 4);
        setLong(isVideo ? (long) (1.0f / mFramerate * 90000 * mFrameCount) : mAudioSample * mAudioFrameCount, tkhd,
                28, 4);
        if (isVideo) {
            setInt(mWidth, tkhd, 84, 2);
            setInt(mHeight, tkhd, 88, 2);
        } else {
            tkhd[23] = 0x02;  // trak id
            tkhd[44] = 0x01;  // 音量
        }

        System.arraycopy(tkhd, 0, tmpBuffer, begin, tkhd.length);

        return tkhd.length;
    }

    private int writeMdia(boolean isVideo, byte[] tmpBuffer, int begin) {
        // 音视频相同
        byte[] mdia = {0x00, 0x00, 0x00, 0x08, 0x6d, 0x64, 0x69, 0x61};

        int boxSize = mdia.length;

        boxSize += writeMdhd(isVideo, tmpBuffer, begin + boxSize);
        boxSize += writeHdlr(isVideo, tmpBuffer, begin + boxSize);
        boxSize += writeMinf(isVideo, tmpBuffer, begin + boxSize);  // 写入trak

        setInt(boxSize, mdia, 0, 4);  // 写入boxsize

        System.arraycopy(mdia, 0, tmpBuffer, begin, mdia.length);  // 写入boxtype

        return boxSize;
    }

    private int writeMdhd(boolean isVideo, byte[] tmpBuffer, int begin) {
        byte[] mdhd = {0x00, 0x00, 0x00, 0x20, 0x6d, 0x64, 0x68, 0x64,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, (byte) 0xe8,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x5f, (byte) 0x90,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

        if (!isVideo) {
            mdhd[20] = 0x00;
            mdhd[21] = 0x00;
            mdhd[22] = 0x1f;
            mdhd[23] = 0x40;
        }
        long duration = isVideo ? (long) (1.0f / mFramerate * 90000 * mFrameCount) : mAudioSample *
                mAudioFrameCount;
        setLong(duration, mdhd, 24, 4);

        System.arraycopy(mdhd, 0, tmpBuffer, begin, mdhd.length);

        return mdhd.length;
    }

    private int writeHdlr(boolean isVideo, byte[] tmpBuffer, int begin) {
        byte[] hdlr = {0x00, 0x00, 0x00, 0x2c, 0x68, 0x64, 0x6c, 0x72,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x76, 0x69, 0x64, 0x65, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00};

        if (!isVideo) {  // soun
            hdlr[16] = 0x73;
            hdlr[17] = 0x6f;
            hdlr[18] = 0x75;
            hdlr[19] = 0x6e;
        }

        System.arraycopy(hdlr, 0, tmpBuffer, begin, hdlr.length);

        return hdlr.length;
    }

    private int writeMinf(boolean isVideo, byte[] tmpBuffer, int begin) {
        // 音视频相同
        byte[] minf = {0x00, 0x00, 0x00, 0x08, 0x6d, 0x69, 0x6e, 0x66};

        int boxSize = minf.length;

        if (isVideo) {
            boxSize += writeVmhd(tmpBuffer, begin + boxSize);
        } else {
            boxSize += writeSmhd(tmpBuffer, begin + boxSize);
        }

        boxSize += writeDinf(tmpBuffer, begin + boxSize);
        boxSize += writeStbl(isVideo, tmpBuffer, begin + boxSize);  // 写入trak

        setInt(boxSize, minf, 0, 4);  // 写入boxsize

        System.arraycopy(minf, 0, tmpBuffer, begin, minf.length);  // 写入boxtype

        return boxSize;
    }

    private int writeVmhd(byte[] tmpBuffer, int begin) {
        byte[] vmhd = {0x00, 0x00, 0x00, 0x14, 0x76, 0x6d, 0x68, 0x64,
                0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00};

        System.arraycopy(vmhd, 0, tmpBuffer, begin, vmhd.length);

        return vmhd.length;
    }

    private int writeSmhd(byte[] tmpBuffer, int begin) {
        byte[] smhd = {0x00, 0x00, 0x00, 0x10, 0x73, 0x6d, 0x68, 0x64,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

        System.arraycopy(smhd, 0, tmpBuffer, begin, smhd.length);

        return smhd.length;
    }

    private int writeDinf(byte[] tmpBuffer, int begin) {
        // 音视频相同
        byte[] dinf = {0x00, 0x00, 0x00, 0x08, 0x64, 0x69, 0x6e, 0x66};

        int boxSize = dinf.length;

        boxSize += writeDref(tmpBuffer, begin + boxSize);

        setInt(boxSize, dinf, 0, 4);  // 写入boxsize

        System.arraycopy(dinf, 0, tmpBuffer, begin, dinf.length);  // 写入boxtype

        return boxSize;
    }

    private int writeDref(byte[] tmpBuffer, int begin) {
        byte[] dref = {0x00, 0x00, 0x00, 0x08, 0x64, 0x72, 0x65, 0x66,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

        int boxSize = dref.length;

        boxSize += writeUrl(tmpBuffer, begin + boxSize);

        setInt(boxSize, dref, 0, 4);  // 写入boxsize

        System.arraycopy(dref, 0, tmpBuffer, begin, dref.length);  // 写入boxtype

        return boxSize;
    }

    private int writeUrl(byte[] tmpBuffer, int begin) {
        byte[] url = {0x00, 0x00, 0x00, 0x0c, 0x75, 0x72, 0x6c, 0x20,
                0x00, 0x00, 0x00, 0x01};

        System.arraycopy(url, 0, tmpBuffer, begin, url.length);

        return url.length;
    }

    private int writeStbl(boolean isVideo, byte[] tmpBuffer, int begin) {
        // 音视频相同
        byte[] stbl = {0x00, 0x00, 0x00, 0x08, 0x73, 0x74, 0x62, 0x6c};

        int boxSize = stbl.length;

        boxSize += writeStts(isVideo, tmpBuffer, begin + boxSize);
        boxSize += writeStsc(tmpBuffer, begin + boxSize);
        boxSize += writeStsd(isVideo, tmpBuffer, begin + boxSize);
        boxSize += writeStsz(isVideo, tmpBuffer, begin + boxSize);
        boxSize += writeCo64(isVideo, tmpBuffer, begin + boxSize);
        if (isVideo) {
            boxSize += writeStss(tmpBuffer, begin + boxSize);
        }

        setInt(boxSize, stbl, 0, 4);  // 写入boxsize

        System.arraycopy(stbl, 0, tmpBuffer, begin, stbl.length);  // 写入boxtype

        return boxSize;
    }

    private int writeStts(boolean isVideo, byte[] tmpBuffer, int begin) {
        byte[] stts = {0x00, 0x00, 0x00, 0x18, 0x73, 0x74, 0x74, 0x73,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}; // 一条记录，简单点

        if (isVideo) {
            setInt(mFrameCount, tmpBuffer, begin + stts.length, 4);

//            long duration = mLastTimetamp / mFrameCount;
            long duration = (long) (1.0f / mFramerate * 90000);
            setLong(duration, tmpBuffer, begin + stts.length + 4, 4);

            System.arraycopy(stts, 0, tmpBuffer, begin, stts.length);

            return stts.length + 8;
        } else {
            setInt(mAudioFrameCount, tmpBuffer, begin + stts.length, 4);

//            long duration = mLastAudioTimestamp / mAudioFrameCount;
            long duration = mAudioSample;
            setLong(duration, tmpBuffer, begin + stts.length + 4, 4);

            System.arraycopy(stts, 0, tmpBuffer, begin, stts.length);

            return stts.length + 8;
//            final List<Long> sampleTimestamps = mAudioSampleTimestamps;
//
//            int sampleTimestampsSize = sampleTimestamps.size();
//
//            int size = (stts.length + sampleTimestampsSize * 8);
//            setInt(size, stts, 0, 4);
//            setInt(sampleTimestampsSize, stts, 12, 4);
//
//            System.arraycopy(stts, 0, tmpBuffer, begin, stts.length);
//
//            for (int i = 0; i < sampleTimestampsSize; ++i) {
//                setInt(1, tmpBuffer, begin + stts.length + i * 8, 4);
//                setLong(sampleTimestamps.get(i), tmpBuffer, begin + stts.length + i * 8 + 4, 4);
//            }
//
//            return size;
        }
    }

    private int writeStsc(byte[] tmpBuffer, int begin) {
        // 音视频相同
        byte[] stsc = {0x00, 0x00, 0x00, 0x1c, 0x73, 0x74, 0x73, 0x63,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
                0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
                0x00, 0x00, 0x00, 0x01};


        System.arraycopy(stsc, 0, tmpBuffer, begin, stsc.length);

        return stsc.length;
    }

    private int writeStsd(boolean isVideo, byte[] tmpBuffer, int begin) {
        // 音视频相同
        byte[] stsd = {0x00, 0x00, 0x00, 0x08, 0x73, 0x74, 0x73, 0x64,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

        int boxSize = stsd.length;

        if (isVideo) {
            boxSize += writeAvc1(tmpBuffer, begin + boxSize);
        } else {
            boxSize += writeMp4a(tmpBuffer, begin + boxSize);
        }

        setInt(boxSize, stsd, 0, 4);  // 写入boxsize

        System.arraycopy(stsd, 0, tmpBuffer, begin, stsd.length);  // 写入boxtype

        return boxSize;
    }

    private int writeAvc1(byte[] tmpBuffer, int begin) {
        byte[] avc1 = {0x00, 0x00, 0x00, 0x56, 0x61, 0x76, 0x63, 0x31,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x00,// 前4个是宽高
                0x00, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x01, 0x0a, 0x41, 0x56, 0x43, 0x20, 0x43,
                0x6f, 0x64, 0x69, 0x6e, 0x67, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x18, (byte) 0xff, (byte) 0xff};

        int boxSize = avc1.length;

        boxSize += writeAvcC(tmpBuffer, begin + boxSize);

        setInt(boxSize, avc1, 0, 4);  // 写入boxsize

        // 写入宽高
        setInt(mWidth, avc1, 32, 2);
        setInt(mHeight, avc1, 34, 2);

        System.arraycopy(avc1, 0, tmpBuffer, begin, avc1.length);  // 写入boxtype

        return boxSize;
    }

    private int writeMp4a(byte[] tmpBuffer, int begin) {
        byte[] mp4a = {0x00, 0x00, 0x00, 0x24, 0x6d, 0x70, 0x34, 0x61,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 全部保留
                0x00, 0x01, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00,
                0x1f, 0x40, 0x00, 0x00};  // sample_rate<<16 8000

        int boxSize = mp4a.length;

        boxSize += writeEsds(tmpBuffer, begin + boxSize);

        setInt(boxSize, mp4a, 0, 4);  // 写入boxsize

        System.arraycopy(mp4a, 0, tmpBuffer, begin, mp4a.length);  // 写入boxtype

        return boxSize;
    }

    private int writeAvcC(byte[] tmpBuffer, int begin) {
        if (mSpsBuffer == null || mPpsBuffer == null) {
            Log.e(TAG, "mSpsBuffer=" + mSpsBuffer + ", mPpsBuffer=" + mPpsBuffer);
            return 0;
        }
        byte[] avcC = {0x00, 0x00, 0x00, 0x10, 0x61, 0x76, 0x63, 0x43,
                0x01, mSpsBuffer[1], mSpsBuffer[2], mSpsBuffer[3], 0x03, 0x01};

        int spsSize = mSpsBuffer.length;
        int ppsSize = mPpsBuffer.length;

        int avcCSize = avcC.length + 2 + spsSize + 2 + ppsSize;

        setInt(avcCSize, avcC, 0, 4);

        int srcBegin = begin;

        System.arraycopy(avcC, 0, tmpBuffer, begin, avcC.length);  // 写入头
        begin = begin + avcC.length;

        setInt(spsSize, tmpBuffer, begin, 2);  // 写入sps长度
        System.arraycopy(mSpsBuffer, 0, tmpBuffer, begin + 2, spsSize);
        begin = begin + 2 + spsSize;

        setInt(ppsSize, tmpBuffer, begin, 2);  // 写入pps长度
        System.arraycopy(mPpsBuffer, 0, tmpBuffer, begin + 2, ppsSize);
        begin = begin + 2 + ppsSize;

        return begin - srcBegin;
    }

    private int writeEsds(byte[] tmpBuffer, int begin) {
        byte[] esds = {0x00, 0x00, 0x00, 0x33, 0x65, 0x73, 0x64, 0x73,
                0x00, 0x00, 0x00, 0x00, 0x03, (byte) 0x80, (byte) 0x80, (byte) 0x80,
                0x22, 0x00, 0x00, 0x1f, 0x04, (byte) 0x80, (byte) 0x80, (byte) 0x80,
                0x14, 0x40, 0x15, 0x00, 0x00, 0x00, 0x00, (byte) 0x80,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, (byte) 0x80,
//                (byte) 0x80, (byte) 0x80, 0x02, 0x14, 0x08, 0x06, (byte) 0x80, (byte) 0x80,  //
// 16000采样
                (byte) 0x80, (byte) 0x80, 0x02, 0x15, (byte) 0x88, 0x06, (byte) 0x80, (byte)
                0x80, // 8000采样
                (byte) 0x80, 0x01, 0x02};


        System.arraycopy(esds, 0, tmpBuffer, begin, esds.length);

        return esds.length;
    }

    private int writeStsz(boolean isVideo, byte[] tmpBuffer, int begin) {
        // 这里的样本大小不包括mdat的头
        byte[] stsz = {0x00, 0x00, 0x00, 0x14, 0x73, 0x74, 0x73, 0x7a,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00};

        final List<Integer> sampleSizes = isVideo ? mVideoSampleSizes : mAudioSampleSizes;

        int sampleSizeSize = sampleSizes.size();

        int size = (stsz.length + sampleSizeSize * 4);
        setInt(size, stsz, 0, 4);
        setInt(sampleSizeSize, stsz, 16, 4);

        System.arraycopy(stsz, 0, tmpBuffer, begin, stsz.length);

        for (int i = 0; i < sampleSizeSize; ++i) {
            setInt(sampleSizes.get(i), tmpBuffer, begin + stsz.length + i * 4, 4);
        }

        return size;
    }

    private int writeCo64(boolean isVideo, byte[] tmpBuffer, int begin) {
        byte[] co64 = {0x00, 0x00, 0x00, 0x10, 0x63, 0x6f, 0x36, 0x34,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

        List<Long> sampleIndexs = isVideo ? mVideoSampleIndexs : mAudioSampleIndexs;

        int sampleIndexSize = sampleIndexs.size();

        int size = (co64.length + sampleIndexSize * 8);
        setInt(size, co64, 0, 4);
        setInt(sampleIndexSize, co64, 12, 4);

        System.arraycopy(co64, 0, tmpBuffer, begin, co64.length);

        for (int i = 0; i < sampleIndexSize; ++i) {
            setLong(sampleIndexs.get(i), tmpBuffer, begin + co64.length + i * 8, 8);
        }

        return size;
    }

    private int writeStss(byte[] tmpBuffer, int begin) {
        // 关键帧
        byte[] stss = {0x00, 0x00, 0x00, 0x10, 0x73, 0x74, 0x73, 0x73,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

        int iFrameSize = mVideoIFrames.size();

        int size = (stss.length + iFrameSize * 4);
        setInt(size, stss, 0, 4);
        setInt(iFrameSize, stss, 12, 4);

        System.arraycopy(stss, 0, tmpBuffer, begin, stss.length);

        for (int i = 0; i < iFrameSize; ++i) {
            setInt(mVideoIFrames.get(i), tmpBuffer, begin + stss.length + i * 4, 4);
        }

        return size;
    }
}
