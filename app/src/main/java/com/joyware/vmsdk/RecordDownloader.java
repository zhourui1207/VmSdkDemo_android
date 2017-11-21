package com.joyware.vmsdk;

import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.support.annotation.NonNull;
import android.util.Log;

import com.joyware.vmsdk.core.Decoder;
import com.joyware.vmsdk.core.RecordThread;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.lang.ref.WeakReference;

import static com.joyware.vmsdk.VmNet.startStream;

/**
 * Created by zhourui on 17/5/26.
 * email: 332867864@qq.com
 * phone: 17006421542
 * 录像下载器
 */

public class RecordDownloader {

    private static final String TAG = "RecordDownloader";

    public static final int RECORD_DOWNLOAD_ERROR_OPEN_STREAM_FAILED = 0;  // 打开码流失败
    public static final int RECORD_DOWNLOAD_ERROR_CONNECT_FAILED = 1;  // 连接失败
    public static final int RECORD_DOWNLOAD_ERROR_STREAM_TIMEOUT = 2;  // 码流超时

    private String mEncryptKey;  // 密钥

    private static final String[] ERROR_MESSAGE = {"打开码流失败", "连接失败", "长时间无码流"};

    private static final int MSG_ID_RECORD_DOWNLOAD_START = 1;
    private static final int MSG_ID_RECORD_DOWNLOAD_PROGRESS = 2;
    //    private static final int MSG_ID_RECORD_DOWNLOAD_STOP = 2;
    private static final int MSG_ID_RECORD_DOWNLOAD_CHECK_TIMEOUT = 3;

    private static final long STREAM_TIMEOUT_MILLIS_SEC = 10000;
    private static final long CHECK_STREAM_TIMEOUT_INTERVAL_MILLIS_SEC = 4000;

    private RecordDownloadHandler mHandler;

    private HandlerThread mHandlerThread;

    private String mFile;

    private OnRecordDownloadStatusListener mOnRecordDownloadStatusListener;

    @NonNull
    private final Object mRecordDownloadStatusListenerMutex = new Object();

    public RecordDownloader() {

    }

    public RecordDownloader(String encrytKey) {
        mEncryptKey = encrytKey;
    }

    public void setOnRecordDownloadStatusListener(OnRecordDownloadStatusListener
                                                          onRecordDownloadStatusListener) {
        synchronized (mRecordDownloadStatusListenerMutex) {
            mOnRecordDownloadStatusListener = onRecordDownloadStatusListener;
        }
    }

    private void onRecordDownloadStart() {
        synchronized (mRecordDownloadStatusListenerMutex) {
            if (mOnRecordDownloadStatusListener != null) {
                mOnRecordDownloadStatusListener.onRecordDownloadStart(this);
            }
        }
    }

    private void onRecordDownloadProgress(float progress) {
        synchronized (mRecordDownloadStatusListenerMutex) {
            if (mOnRecordDownloadStatusListener != null) {
                mOnRecordDownloadStatusListener.onRecordDownloadProgress(this, progress);
            }
        }
    }

    private void onRecordDownloadSuccess() {
        synchronized (mRecordDownloadStatusListenerMutex) {
            if (mOnRecordDownloadStatusListener != null) {
                mOnRecordDownloadStatusListener.onRecordDownloadSuccess(this);
            }
        }
    }

    private void onRecordDownloadFailed(int errorCode, String message) {
        synchronized (mRecordDownloadStatusListenerMutex) {
            if (mOnRecordDownloadStatusListener != null) {
                mOnRecordDownloadStatusListener.onRecordDownloadFailed(this, errorCode, message);
            }
        }
    }

    private void threadQuitSelfLock() {
        synchronized (this) {
            mHandlerThread = null;
            mHandler = null;
            mFile = null;
        }
    }

    /**
     * 开始录像任务
     *
     * @param fdId      设备序列号
     * @param channelId 通道号
     * @param beginTime 开始时间
     * @param endTime   结束实际那
     * @return true 成功；false 失败
     */
    public boolean startRecordTask(int recordType, String file, boolean center, String fdId, int
            channelId, int beginTime, int endTime) {
        synchronized (this) {
            if (mHandlerThread != null) {
                return false;
            }

            mHandlerThread = new HandlerThread("RecordDownloadThread");
            mHandlerThread.start();

            mHandler = new RecordDownloadHandler(mHandlerThread.getLooper(), new
                    WeakReference<RecordDownloader>(this), recordType, file, center, fdId,
                    channelId, beginTime, endTime, mEncryptKey);


            mFile = file;

            // 发送开始消息
            mHandler.sendEmptyMessage(MSG_ID_RECORD_DOWNLOAD_START);

            return true;
        }
    }

    public boolean cancelRecordTask() {
        synchronized (this) {
            if (mHandlerThread == null) {
                return false;
            }

            if (mHandler != null) {
                mHandler.stopCheckStreamTimeout();
                mHandler.removeMessages(MSG_ID_RECORD_DOWNLOAD_START);
                mHandler.removeMessages(MSG_ID_RECORD_DOWNLOAD_PROGRESS);

                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
                    mHandler.getLooper().quitSafely();
                } else {
                    mHandler.getLooper().quit();
                }

                mHandler.quit(false);
                mHandler = null;
            }

            mHandlerThread.interrupt();
            try {
                mHandlerThread.join();
            } catch (InterruptedException e) {
                Log.e(TAG, "HandlerThread.join() throws InterruptedException! message:" + e
                        .getMessage());
            }

            mHandlerThread = null;

            return true;
        }
    }

    public String file() {
        synchronized (this) {
            return mFile;
        }
    }

    public boolean downloading() {
        synchronized (this) {
            return mHandlerThread != null;
        }
    }

    private static class RecordDownloadHandler extends Handler {
        @NonNull
        private WeakReference<RecordDownloader> mRecordDownloaderWeakReference;
        @NonNull
        private Decoder mDecoder;  // 解码器，拼帧，去ps头使用
        @NonNull
        private RecordThread mRecordThread;  // 录像线程，写录像使用

        private int mRecordType;
        private String mFile;
        private boolean mCenter;
        private String mFdId;
        private int mChannelId;
        private int mBeginTime;
        private int mEndTime;
        private long mTotalMillisSec;
        private float mCurrentPercent = -1f;  // 当前百分比

        private int mMonitorId;
        private int mVideoStreamId;
        private int mAudioStreamId;

        private boolean mCheckTimeout;
        private long mLastReceiveStreamDataTime;  // 最后接收码流时间

        private OutputStream mOutputStream;
        private boolean mSaveStream = false;

        RecordDownloadHandler(Looper looper, @NonNull WeakReference<RecordDownloader>
                recordDownloaderWeakReference, int recordType, String file, boolean center,
                              String fdId, int channelId, int beginTime, int endTime,
                              String encryptKey) {
            super(looper);
            mRecordDownloaderWeakReference = recordDownloaderWeakReference;
            mRecordType = recordType;
            mFile = file;
            mCenter = center;
            mFdId = fdId;
            mChannelId = channelId;
            mBeginTime = beginTime;
            mEndTime = endTime;
            mTotalMillisSec = (endTime - beginTime) * 1000L;
            mDecoder = new Decoder(VmType.DECODE_TYPE_INTELL, false, false, false, encryptKey, 0, null);
            mDecoder.setOnESFrameDataCallback(new Decoder.OnESFrameDataCallback() {
                @Override
                public void onFrameData(boolean video, int timestamp, long pts, byte[] data, int
                        start, int len) {
//                    Log.e(TAG, "video=" + video + ", timestamp=" + timestamp + "， len=" + len);
                    mRecordThread.write(video, timestamp, pts, data, start, len);
                }
            });
            mRecordThread = new RecordThread();
            mRecordThread.setOnRecordStatusListener(new RecordThread.OnRecordStatusListener() {
                @Override
                public void onRecordBegin() {
//                    sendEmptyMessage(MSG_ID_RECORD_START);
                }

                @Override
                public void onRecordProgress(long millisTime) {
//                    Log.e(TAG, "onRecordProgress millisTime=" + millisTime);
                    Message message = Message.obtain();
                    message.what = MSG_ID_RECORD_DOWNLOAD_PROGRESS;
                    message.obj = millisTime;
                    sendMessage(message);
                }

                @Override
                public void onRecordEnd() {

                }
            });
            mRecordThread.startRecord(mRecordType, mFile);
        }

        @Override
        public void handleMessage(Message msg) {
            RecordDownloader recordDownloader = mRecordDownloaderWeakReference.get();
            if (recordDownloader == null) {
                return;
            }

//            Log.e(TAG, "start handle msg, what=" + msg.what);
            switch (msg.what) {
                case MSG_ID_RECORD_DOWNLOAD_START:
                    final PlayAddressHolder playAddressHolder = new PlayAddressHolder();
                    int ret = VmNet.openPlaybackStream(mFdId, mChannelId, mCenter, mBeginTime,
                            mEndTime, playAddressHolder);

                    if (ret == 0 && playAddressHolder.getMonitorId() != 0 && playAddressHolder
                            .getVideoAddr() != null && playAddressHolder.getVideoPort() > 0) {
                        mMonitorId = playAddressHolder.getMonitorId();

                        String videoAddr = playAddressHolder.getVideoAddr();
                        int videoPort = playAddressHolder.getVideoPort();

                        StreamIdHolder streamIdHolder = new StreamIdHolder();
                        ret = startStream(videoAddr, videoPort, new VmNet.StreamCallback() {
                            @Override
                            public void onStreamConnectStatus(int streamId, boolean isConnected) {

                            }

                            @Override
                            public void onReceiveStream(int streamId, int streamType, int
                                    payloadType, byte[] buffer, int timeStamp, int seqNumber,
                                                        boolean isMark) {
                                // 记录最后接收码流时间
                                mLastReceiveStreamDataTime = System.currentTimeMillis();
//                                Log.e(TAG, "onReceiveStream streamType=" + streamType + ", " +
//                                        "payloadType=" + payloadType + ", timeStamp=" + timeStamp
//                                        + ", seqNumber=" + seqNumber + ", len=" + buffer.length + ", mark=" + isMark);

                                if (mSaveStream) {
                                    if (mOutputStream == null) {
                                        try {
                                            mOutputStream = new FileOutputStream("/sdcard/MVSS/stream" +
                                                    ".text");
                                        } catch (FileNotFoundException e) {
                                            e.printStackTrace();
                                        }
                                    }

                                    if (mOutputStream != null) {
                                        try {
                                            byte[] aa = {0x00, 0x00, 0x00, 0x02};
                                            mOutputStream.write(aa);
                                            mOutputStream.write(buffer);
                                        } catch (IOException e) {
                                            e.printStackTrace();
                                        }
                                    }
                                }

                                mDecoder.addBuffer(streamId, VmType.STREAM_TYPE_VIDEO, payloadType, buffer,
                                        0, buffer.length, timeStamp, seqNumber, isMark, false, 0L);
                            }
                        }, streamIdHolder);

                        if (ret == 0 && streamIdHolder.getStreamId() != 0) {
                            mVideoStreamId = streamIdHolder.getStreamId();
                            // 最快速度发送
                            VmNet.controlPlayback(mMonitorId, 0, "SPEED", "128");
                            // 开启码流超时检测
                            startCheckStreamTimeout();
                        } else {
                            Log.e(TAG, "startStream failed. ret = " + ret);
                            recordDownloader.onRecordDownloadFailed
                                    (RECORD_DOWNLOAD_ERROR_CONNECT_FAILED,
                                            ERROR_MESSAGE[RECORD_DOWNLOAD_ERROR_CONNECT_FAILED]);

                            recordDownloader.threadQuitSelfLock();
                        }

                        String audioAddr = playAddressHolder.getAudioAddr();
                        int audioPort = playAddressHolder.getAudioPort();
                        streamIdHolder.setStreamId(0);
                        if (audioAddr != null && !audioAddr.isEmpty() && audioPort > 0) {
                            ret = VmNet.startStream(audioAddr, audioPort, new VmNet
                                    .StreamCallback() {
                                @Override
                                public void onStreamConnectStatus(int streamId, boolean
                                        isConnected) {

                                }

                                @Override
                                public void onReceiveStream(int streamId, int streamType, int
                                        payloadType, byte[] buffer, int timeStamp, int seqNumber,
                                                            boolean isMark) {
//                                    Log.e(TAG, "onReceiveStream streamType=" + streamType + ", " +
//                                            "payloadType=" + payloadType + ", timeStamp=" +
//                                            timeStamp);
//                                    mDecoder.addBuffer(streamId, VmType.STREAM_TYPE_AUDIO,
//                                            payloadType, buffer,
//                                            0, buffer.length, timeStamp, seqNumber, isMark);
                                }
                            }, streamIdHolder);

                            if (ret == 0 && streamIdHolder.getStreamId() != 0) {
                                mAudioStreamId = streamIdHolder.getStreamId();
                            }
                        }

                    } else {
                        Log.e(TAG, "openPlaybackStream failed. ret = " + ret);
                        recordDownloader.onRecordDownloadFailed
                                (RECORD_DOWNLOAD_ERROR_OPEN_STREAM_FAILED,
                                        ERROR_MESSAGE[RECORD_DOWNLOAD_ERROR_OPEN_STREAM_FAILED]);

                        recordDownloader.threadQuitSelfLock();
                    }

                    break;

                case MSG_ID_RECORD_DOWNLOAD_PROGRESS:
                    long offsetMillisSec = (long) msg.obj;
                    mCurrentPercent = 1f * offsetMillisSec / mTotalMillisSec;

                    if (mCurrentPercent > 1f) {
                        mCurrentPercent = 1f;
                    }
                    Log.w(TAG, "mCurrentPercent=" + mCurrentPercent);
                    recordDownloader.onRecordDownloadProgress(mCurrentPercent);
                    break;

                case MSG_ID_RECORD_DOWNLOAD_CHECK_TIMEOUT:
                    // 超时检测
                    Log.w(TAG, "Record download thread check stream timeout!");
                    if (mLastReceiveStreamDataTime != 0 && (System.currentTimeMillis() -
                            mLastReceiveStreamDataTime > STREAM_TIMEOUT_MILLIS_SEC)) {
                        if (mTotalMillisSec < 10) {
                            if (mCurrentPercent >= 0.2f || mCurrentPercent == 0f) {  //
                                // 当前进度大于10%，认为任务完成，或者压根就没进度
                                quit(true);
                                recordDownloader.onRecordDownloadSuccess();
                                recordDownloader.threadQuitSelfLock();
                            } else {  // 任务网络超时
                                quit(false);
                                recordDownloader.onRecordDownloadFailed
                                        (RECORD_DOWNLOAD_ERROR_STREAM_TIMEOUT,
                                                ERROR_MESSAGE[RECORD_DOWNLOAD_ERROR_STREAM_TIMEOUT]);
                                recordDownloader.threadQuitSelfLock();
                            }
                        } else if (mTotalMillisSec <= 30) {
                            if (mCurrentPercent >= 0.5f || mCurrentPercent == 0f) {  //
                                // 当前进度大于10%，认为任务完成，或者压根就没进度
                                quit(true);
                                recordDownloader.onRecordDownloadSuccess();
                                recordDownloader.threadQuitSelfLock();
                            } else {  // 任务网络超时
                                quit(false);
                                recordDownloader.onRecordDownloadFailed
                                        (RECORD_DOWNLOAD_ERROR_STREAM_TIMEOUT,
                                                ERROR_MESSAGE[RECORD_DOWNLOAD_ERROR_STREAM_TIMEOUT]);
                                recordDownloader.threadQuitSelfLock();
                            }
                        } else if (mTotalMillisSec <= 600) {
                            if (mCurrentPercent >= 0.65f || mCurrentPercent == 0f) {  //
                                // 当前进度大于10%，认为任务完成，或者压根就没进度
                                quit(true);
                                recordDownloader.onRecordDownloadSuccess();
                                recordDownloader.threadQuitSelfLock();
                            } else {  // 任务网络超时
                                quit(false);
                                recordDownloader.onRecordDownloadFailed
                                        (RECORD_DOWNLOAD_ERROR_STREAM_TIMEOUT,
                                                ERROR_MESSAGE[RECORD_DOWNLOAD_ERROR_STREAM_TIMEOUT]);
                                recordDownloader.threadQuitSelfLock();
                            }
                        } else {
                            if (mCurrentPercent >= 0.90f || mCurrentPercent == 0f) {  //
                                // 当前进度大于95%，认为任务完成，或者压根就没进度
                                quit(true);
                                recordDownloader.onRecordDownloadSuccess();
                                recordDownloader.threadQuitSelfLock();
                            } else {  // 任务网络超时
                                quit(false);
                                recordDownloader.onRecordDownloadFailed
                                        (RECORD_DOWNLOAD_ERROR_STREAM_TIMEOUT,
                                                ERROR_MESSAGE[RECORD_DOWNLOAD_ERROR_STREAM_TIMEOUT]);
                                recordDownloader.threadQuitSelfLock();
                            }
                        }
                    } else if (mCheckTimeout) {
                        startCheckStreamTimeout();
                    }

                    break;
            }

//            Log.e(TAG, "end handle msg, what=" + msg.what);
        }

        private void startCheckStreamTimeout() {
            Log.w(TAG, "start check stream timeout");
            mCheckTimeout = true;
            if (mLastReceiveStreamDataTime == 0) {
                mLastReceiveStreamDataTime = System.currentTimeMillis();
            }
            sendEmptyMessageDelayed(MSG_ID_RECORD_DOWNLOAD_CHECK_TIMEOUT,
                    CHECK_STREAM_TIMEOUT_INTERVAL_MILLIS_SEC);
        }

        private void stopCheckStreamTimeout() {
            mCheckTimeout = false;
            mLastReceiveStreamDataTime = 0;
            removeMessages(MSG_ID_RECORD_DOWNLOAD_CHECK_TIMEOUT);
        }

        private void quit(boolean createFile) {
            if (mOutputStream != null) {
                try {
                    mOutputStream.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }

            if (mVideoStreamId != 0) {
                VmNet.stopStream(mVideoStreamId);
                mVideoStreamId = 0;
            }
            if (mAudioStreamId != 0) {
                VmNet.stopStream(mAudioStreamId);
                mAudioStreamId = 0;
            }
            if (mMonitorId != 0) {
                VmNet.closePlaybackStream(mMonitorId);
                mMonitorId = 0;
            }

            mDecoder.stopPlay();
            mRecordThread.stopRecord(createFile);
        }
    }

    // 非主线程
    public interface OnRecordDownloadStatusListener {
        void onRecordDownloadStart(RecordDownloader recordDownloader);

        void onRecordDownloadProgress(RecordDownloader recordDownloader, float progress);

        void onRecordDownloadSuccess(RecordDownloader recordDownloader);

        void onRecordDownloadFailed(RecordDownloader recordDownloader, int errorCode, String
                message);
    }
}
