package com.joyware.vmsdk.core;

import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.support.annotation.NonNull;
import android.util.Log;

import com.joyware.vmsdk.util.FileUtil;
import com.joyware.vmsdk.util.FlvSave;
import com.joyware.vmsdk.util.Mp4Save;

import java.io.FileNotFoundException;
import java.lang.ref.WeakReference;

/**
 * Created by zhourui on 17/5/25.
 * email: 332867864@qq.com
 * phone: 17006421542
 */

public class RecordThread {

    private static final String TAG = "RecordThread";

    // 录像文件类型
    public static final int RECORD_FILE_TYPE_MP4 = 0;  // MP4文件
    public static final int RECORD_FILE_TYPE_FLV = 1;  // flv文件

    private static final int MSG_ID_START_RECORD = 0;  // 开始录像
    private static final int MSG_ID_WRITE_RECORD = 1;  // 写录像
    private static final int MSG_ID_STOP_RECORD = 2;  // 停止录像

    private int mFileType = RECORD_FILE_TYPE_MP4;  // 默认使用MP4格式

    private HandlerThread mHandlerThread;

    private RecordHandler mHandler;

    private String mFile;

    private Mp4Save mMp4Save;

    private FlvSave mFlvSave;

    private boolean mSaveSuccess;

    private OnRecordStatusListener mOnRecordStatusListener;

    public void setOnRecordStatusListener(OnRecordStatusListener onRecordStatusListener) {
        mOnRecordStatusListener = onRecordStatusListener;
    }

    public int getFileType() {
        return mFileType;
    }

    public String getFile() {
        return mFile;
    }

    public boolean recording() {
        synchronized (this) {
            return mHandlerThread != null;
        }
    }

    /**
     * 开始录像
     *
     * @param fileType 录像文件类型，支持MP4和FLV
     * @param file     录像文件路径
     * @return true：成功；false：失败
     */
    public boolean startRecord(int fileType, String file) {
        synchronized (this) {
            if (mHandlerThread != null) {
                return false;
            }

            if (fileType < RECORD_FILE_TYPE_MP4 || fileType > RECORD_FILE_TYPE_FLV) {
                throw new IllegalArgumentException("fileType:" + fileType + " is not exist!");
            }

            mFileType = fileType;

            // 尝试文件是否能创建和访问
            if (!FileUtil.cratePath(file)) {
                return false;
            }

            try {
                if (mFileType == RECORD_FILE_TYPE_MP4) {
                    mMp4Save = new Mp4Save(file);
                } else if (mFileType == RECORD_FILE_TYPE_FLV) {
                    mFlvSave = new FlvSave(file);
                }
            } catch (FileNotFoundException e) {
                Log.e(TAG, "HandlerThread.join() throws FileNotFoundException! message:" + e
                        .getMessage());
            }

            // 保存文件名，上层可以使用get方法获取
            mFile = file;

            // 创建线程
            mHandlerThread = new HandlerThread("RecordHandlerThread");
            mHandlerThread.start();

            // 创建handler
            mHandler = new RecordHandler(mHandlerThread.getLooper(), new
                    WeakReference<RecordThread>(this));

            // 发送开始录像消息
            mHandler.sendEmptyMessage(MSG_ID_START_RECORD);

            return true;
        }
    }

    public boolean stopRecord(boolean createFile) {
        synchronized (this) {
            if (mHandler == null) {
                return false;
            }

            // 移除所有写操作
            mHandler.removeMessages(MSG_ID_WRITE_RECORD);
            // 增加结束操作
            Message message = Message.obtain();
            message.what = MSG_ID_STOP_RECORD;
            message.obj = createFile;
            mHandler.sendMessage(message);

            // 等待线程结束
            if (mHandlerThread != null) {
                try {
                    mHandlerThread.join();
                } catch (InterruptedException e) {
                    Log.e(TAG, "HandlerThread.join() throws InterruptedException! message:" + e
                            .getMessage());
                }
            }

            // 释放
            mHandlerThread = null;
            mHandler = null;
            mFlvSave = null;
            mMp4Save = null;
            mFile = null;
            final boolean saveSuccess = mSaveSuccess;
            mSaveSuccess = false;

            return saveSuccess;
        }
    }

    public boolean write(boolean video, int timestamp, long pts, byte[] data, int start, int len) {
        synchronized (this) {
            if (mHandler == null) {
                return false;
            }
            RecordData recordData = new RecordData(video, timestamp, pts, data, start, len);
            Message message = Message.obtain();
            message.what = MSG_ID_WRITE_RECORD;
            message.obj = recordData;
            mHandler.sendMessage(message);
            return true;
        }
    }

    private void onRecordBegin() {
        if (mOnRecordStatusListener != null) {
            mOnRecordStatusListener.onRecordBegin();
        }
    }

    private void onRecordProgress(long millisSec) {
        if (mOnRecordStatusListener != null) {
            mOnRecordStatusListener.onRecordProgress(millisSec);
        }
    }

    private void onRecordEnd() {
        if (mOnRecordStatusListener != null) {
            mOnRecordStatusListener.onRecordEnd();
        }
    }

    private static class RecordHandler extends Handler {

        @NonNull
        private WeakReference<RecordThread> mRecordThreadWeakReference;

        private boolean mFirst = true;
        private boolean mGetConf;
        private byte[] mSPS;
        private byte[] mPPS;
        private byte[] mIFrame;
        private byte[] mPFrame;
        private int mWidth;
        private int mHeight;
        private int mFramerate = 25;
        private boolean mCross;  // 是否是交叉模式，交叉模式下，两个nalu合并为一帧

        private boolean mWriteConf;
        private boolean mUsePTS = true;
        private long mFirstTimestamp;
        private long mTimestamp;

        private long mAACEncoder;
        private byte[] mAudioDecodeBuffer;

        RecordHandler(@NonNull WeakReference<RecordThread> recordThreadWeakReference) {
            mRecordThreadWeakReference = recordThreadWeakReference;
        }

        RecordHandler(Looper looper, @NonNull WeakReference<RecordThread>
                recordThreadWeakReference) {
            super(looper);
            mRecordThreadWeakReference = recordThreadWeakReference;
        }

        private void initAACEncoder() {
            if (mAACEncoder == 0) {
                mAACEncoder = Decoder.AACEncoderInit(8000);
            }
        }

        private void uninitAACEncoder() {
            if (mAACEncoder != 0) {
                Decoder.AACEncoderUninit(mAACEncoder);
                mAACEncoder = 0;
            }
        }

        @Override
        public void handleMessage(Message msg) {
            RecordThread recordThread = mRecordThreadWeakReference.get();
            if (recordThread == null) {
                return;
            }

            switch (msg.what) {
                case MSG_ID_START_RECORD:
                    recordThread.onRecordBegin();
                    break;
                case MSG_ID_WRITE_RECORD:
                    RecordData recordData = (RecordData) msg.obj;

                    byte[] data = recordData.getData();
                    if (data.length < 5) {
                        return;
                    }

                    boolean video = recordData.isVideo();
                    int naluType = data[4] & 0x1F;

//                    Log.e(TAG, "naluType = " + naluType);

                    if (video && !mGetConf) {
                        if (naluType == 7) {  // sps
                            if (mSPS == null) {
                                mSPS = new byte[data.length];
                                System.arraycopy(data, 0, mSPS, 0, data.length);
                            }
                            mIFrame = null;
                        } else if (naluType == 8) {  // pps
                            if (mPPS == null) {
                                mPPS = new byte[data.length];
                                System.arraycopy(data, 0, mPPS, 0, data.length);
                            }
                            mIFrame = null;
                        } else if (naluType == 5) {  // Iframe
                            if (mIFrame == null) {
                                mIFrame = new byte[data.length];
                                System.arraycopy(data, 0, mIFrame, 0, data.length);
                            }
                        } else if (naluType == 1) {
                            if (mIFrame != null) {
                                mPFrame = new byte[data.length];
                                System.arraycopy(data, 0, mPFrame, 0, data.length);
                            }
                        }

                        decodeConf();
                    }

                    if (recordThread.mFileType == RECORD_FILE_TYPE_MP4 && recordThread.mMp4Save
                            != null) {
                        if (video) {
                            if (!mWriteConf && mGetConf) {
                                recordThread.mMp4Save.writeStart();
                                recordThread.mMp4Save.writeConfiguretion(mWidth, mHeight,
                                        mCross ? mFramerate * 2 : mFramerate, mSPS, 0, mSPS.length, mPPS, 0, mPPS.length);
                                mSPS = null;
                                mPPS = null;
                                mWriteConf = true;
                            }

                            if (mWriteConf && naluType != 7 && naluType != 8 && naluType != 6) {
                                if (mFirstTimestamp == 0) {
                                    // 优先使用pts
                                    if (recordData.getPTS() != 0) {
                                        mFirstTimestamp = mTimestamp = recordData.getPTS();
                                    } else if (recordData.getTimestamp() > 0) {
                                        mFirstTimestamp = mTimestamp = recordData.getTimestamp()
                                                * 1000L;
                                        mUsePTS = false;
                                    }
                                } else {
                                    // 优先使用pts
                                    if (mUsePTS && recordData.getPTS() != 0) {
                                        mTimestamp = recordData.getPTS();
                                    } else if (!mUsePTS && recordData.getTimestamp() > 0) {
                                        mTimestamp = recordData.getTimestamp() * 1000L;
//                                        Log.e(TAG, "mTimestamp=" + mTimestamp);
                                    }
                                }

                                if (mFirst) { // 第一帧要保证是I帧
                                    if (naluType == 5) {
                                        recordThread.mMp4Save.writeNalu(true, data, 0, data.length);
                                    } else if (mIFrame != null) {
                                        recordThread.mMp4Save.writeNalu(true, mIFrame, 0, mIFrame.length);
                                        recordThread.mMp4Save.writeNalu(false, data, 0, data.length);
                                    }
                                    mFirst = false;
                                } else {
                                    recordThread.mMp4Save.writeNalu(naluType == 5, data, 0, data.length);
                                }
                            }
                        } else {
                            initAACEncoder();
                            if (mAACEncoder != 0) {
                                if (mAudioDecodeBuffer == null) {
                                    mAudioDecodeBuffer = new byte[2048 * 2];
                                }
                                int decodeLen = Decoder.AACEncodePCM2AAC(mAACEncoder, data, data
                                        .length, mAudioDecodeBuffer);
                                if (!mFirst && mGetConf && decodeLen > 0) {  // 转aac成功
                                    recordThread.mMp4Save.writeAAC(mAudioDecodeBuffer, 0,
                                            decodeLen);
                                }
                            }
                        }

                    } else if (recordThread.mFileType == RECORD_FILE_TYPE_FLV && recordThread
                            .mFlvSave != null) {
                        if (mFirst) {
                            recordThread.mFlvSave.writeStart(true);
                            mFirst = false;
                        }
                        if (video) {
                            if (!mWriteConf && mGetConf) {
                                recordThread.mFlvSave.writeConfiguretion(mSPS, 0, mSPS.length, mPPS,
                                        0, mPPS.length);
                                mSPS = null;
                                mPPS = null;
                                mWriteConf = true;
                            }

                            if (mWriteConf && naluType != 7 && naluType != 8) {
                                recordThread.mFlvSave.writeNalu(naluType == 5, data, 0, data
                                        .length);
                            }
                        } else {
                            // flv暂不支持音频
                        }
                    }
                    // 对调进度，毫秒
                    long offsetMillisSec = mUsePTS ? (long) ((mTimestamp - mFirstTimestamp) /
                            90f) : (mTimestamp - mFirstTimestamp);
                    recordThread.onRecordProgress(offsetMillisSec);
                    break;
                case MSG_ID_STOP_RECORD:
                    boolean createFile = (boolean) msg.obj;

                    if (recordThread.mFileType == RECORD_FILE_TYPE_MP4 && recordThread.mMp4Save
                            != null) {
                        if (createFile) {
                            recordThread.mSaveSuccess = recordThread.mMp4Save.save();
                        } else {
                            recordThread.mMp4Save.cancel();
                        }
                    } else if (recordThread.mFileType == RECORD_FILE_TYPE_FLV && recordThread
                            .mFlvSave != null) {
                        if (createFile) {
                            recordThread.mSaveSuccess = recordThread.mFlvSave.save();
                        } else {
                            recordThread.mFlvSave.cancel();
                        }
                    }
                    recordThread.onRecordEnd();

                    uninitAACEncoder();
                    mAudioDecodeBuffer = null;
                    getLooper().quit();
                    break;
            }
        }

        // 获取视频信息
        private void decodeConf() {
            if (!mGetConf && mSPS != null && mPPS != null && mIFrame != null && mPFrame != null) {
                FrameConfHolder frameConfHolder = new FrameConfHolder();
                long decodeConfDecoderHandle = Decoder.DecoderInit(98);
                Decoder.DecodeNalu2RGB(decodeConfDecoderHandle, mSPS, mSPS.length, null,
                        frameConfHolder);
                Decoder.DecodeNalu2RGB(decodeConfDecoderHandle, mPPS, mPPS.length, null,
                        frameConfHolder);
                int ret = Decoder.DecodeNalu2RGB(decodeConfDecoderHandle, mIFrame, mIFrame
                        .length, null, frameConfHolder);
                if (ret <= 0) {
                    mCross = true;
                    ret = Decoder.DecodeNalu2RGB(decodeConfDecoderHandle, mPFrame, mPFrame
                            .length, null, frameConfHolder);
                } else {
                    mCross = false;
                }

                Decoder.DecoderUninit(decodeConfDecoderHandle);

                if (ret > 0) {
                    mWidth = frameConfHolder.getWidth();
                    mHeight = frameConfHolder.getHeight();
                    mFramerate = frameConfHolder.getFramerate();
                    Log.w(TAG, "获取视频信息成功! width=" + mWidth + ", height=" + mHeight + ", " +
                            "framerate=" + mFramerate);

//                    mIFrame = null;
//                    mPFrame = null;
                    mGetConf = true;
                } else {
                    Log.e(TAG, "获取视频信息失败！");
                }
            }
        }

    }

    // 录像状态侦听，此状态回调是子线程中调用
    public interface OnRecordStatusListener {
        // 录像开始
        void onRecordBegin();

        // 录像进度
        void onRecordProgress(long millisTime);

        // 录像结束
        void onRecordEnd();
    }

    private class RecordData {
        boolean mVideo;
        int mTimestamp;
        long mPTS;
        byte[] mData;

        RecordData(boolean video, int timestamp, long pts, byte[] data, int start, int len) {
            mVideo = video;
            mTimestamp = timestamp;
            mPTS = pts;
            mData = new byte[len];
            System.arraycopy(data, start, mData, 0, len);
        }

        public boolean isVideo() {
            return mVideo;
        }

        public byte[] getData() {
            return mData;
        }

        public int getTimestamp() {
            return mTimestamp;
        }

        public long getPTS() {
            return mPTS;
        }
    }
}
