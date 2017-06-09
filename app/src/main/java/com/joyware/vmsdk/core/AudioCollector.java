package com.joyware.vmsdk.core;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.media.MediaRecorder;
import android.util.Log;

import com.joyware.vmsdk.util.G711;

import java.lang.ref.WeakReference;

/**
 * Created by zhourui on 17/5/4.
 * email: 332867864@qq.com
 * phone: 17006421542
 * 声音采集器
 */

public class AudioCollector {

    private static final String TAG = "AudioCollector";

    private static int mFrequency = 8000;
    private static int mChannelConfiguration = AudioFormat.CHANNEL_CONFIGURATION_MONO;
    private static int mAudioEncoding = AudioFormat.ENCODING_PCM_16BIT;
    private AudioRecordThread mAudioRecordThread;

    private static int mRecBufSize, mPlayBufSize;

    private AudioRecord mAudioRecord;

    private AudioTrack mAudioTrack;

    private boolean mPlayAudio;

    private OnG711AudioDataListener mOnG711AudioDataListener;

    private int mSendBufSize;

    public AudioCollector() {
        init(320);
    }

    public AudioCollector(int sendBufSize) {
        init(sendBufSize);
    }

    private void init(int sendBufSize) {
        if (sendBufSize <= 0) {
            throw new IllegalArgumentException("sendBufSize must > 0");
        }
        mSendBufSize = sendBufSize;

        //用getMinBufferSize()方法得到采集数据所需要的最小缓冲区的大小
        mRecBufSize = AudioRecord.getMinBufferSize(mFrequency, mChannelConfiguration,
                mAudioEncoding);

        mPlayBufSize = AudioTrack.getMinBufferSize(mFrequency, mChannelConfiguration,
                mAudioEncoding);

        Log.w(TAG, "receive buffer size=" + mRecBufSize + ", play buffer size=" + mPlayBufSize);
    }

    public void setPlay(boolean play) {
        mPlayAudio = play;
    }

    public boolean isRecording() {
        return mAudioRecord != null && mAudioRecord.getRecordingState() == AudioRecord.RECORDSTATE_RECORDING;
    }

    public void setOnAudioDataListener(OnG711AudioDataListener onG711AudioDataListener) {
        mOnG711AudioDataListener = onG711AudioDataListener;
    }

    private AudioTrack createAudioTrack() {
        if (mAudioTrack == null) {
            int maxjitter = AudioTrack.getMinBufferSize(mFrequency, mChannelConfiguration,
                    mAudioEncoding);
            mAudioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, mFrequency,
                    mChannelConfiguration, mAudioEncoding,
                    maxjitter, AudioTrack.MODE_STREAM);
            mAudioTrack.play();
        }
        return mAudioTrack;
    }

    private void releaseAudioTrack() {
        if (mAudioTrack != null) {
            mAudioTrack.stop();
            mAudioTrack.release();
            mAudioTrack = null;
        }
    }

    private AudioRecord createAudioRecord() {
        if (mAudioRecord == null) {
            //实例化AudioRecord(声音来源，采样率，声道设置，采样声音编码，缓存大小）
            mAudioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC, mFrequency,
                    mChannelConfiguration, mAudioEncoding, mRecBufSize);
        }
        return mAudioRecord;
    }

    private void releaseAudioRecord() {
        mAudioRecord.stop();
        mAudioRecord.release();
        mAudioRecord = null;
    }

    public boolean startRecord() {
        if (mAudioRecordThread == null) {
            mAudioRecordThread = new AudioRecordThread(new WeakReference<AudioCollector>(this));
            mAudioRecordThread.start();
            return true;
        }
        return false;
    }

    public void stopRecord() {
        if (mAudioRecordThread != null) {
            mAudioRecordThread.interrupt();
            try {
                mAudioRecordThread.join();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            mAudioRecordThread = null;
        }
    }

    private static class AudioRecordThread extends Thread {

        private static final String TAG = "AudioRecordThread";

        private WeakReference<AudioCollector> mAudioCollectorWeakReference;

        private AudioRecordThread(WeakReference<AudioCollector> audioCollectorWeakReference) {
            if (audioCollectorWeakReference == null) {
                throw new NullPointerException("audioCollectorWeakReference can not be null!");
            }
            mAudioCollectorWeakReference = audioCollectorWeakReference;
        }

        @Override
        public void run() {
            try {
                loop();
            } catch (InterruptedException e) {
                Log.e(TAG, "audio record thread interrupted！" + e.getMessage());
            }
        }

        private void loop() throws InterruptedException {
            AudioCollector audioCollector = mAudioCollectorWeakReference.get();
            if (audioCollector == null) {
                return;
            }

            byte[] buffer = new byte[audioCollector.mSendBufSize * 2];  // 要求是发25帧 8000*2/25=640
            byte[] encodeBuf = new byte[buffer.length];
            byte[] playAudioBuf = new byte[mPlayBufSize];
            int playAudioPosition = 0;

            AudioRecord audioRecord = audioCollector.createAudioRecord();
            audioRecord.startRecording();

            while (!isInterrupted()) {
                int bufferReadResult = audioRecord.read(buffer, 0, buffer.length);
                if (bufferReadResult > 0) {
                    if (audioCollector.mPlayAudio) {
                        if (playAudioPosition + bufferReadResult > mPlayBufSize) {  // 超过最大播放缓存大小
                            int len = mPlayBufSize - playAudioPosition;
                            int leaveLen = bufferReadResult - len;
                            System.arraycopy(buffer, 0, playAudioBuf, playAudioPosition, len);

                            AudioTrack audioTrack = audioCollector.createAudioTrack();
                            audioTrack.write(playAudioBuf, 0, playAudioBuf.length);
                            playAudioPosition = 0;

                            System.arraycopy(buffer, len, playAudioBuf, playAudioPosition, leaveLen);
                            playAudioPosition += leaveLen;
                        } else {  // 没超过最大缓存大小
                            System.arraycopy(buffer, 0, playAudioBuf, playAudioPosition, bufferReadResult);
                            playAudioPosition += bufferReadResult;
                        }

                        if (playAudioPosition == mPlayBufSize) {  // 需要播放了
                            AudioTrack audioTrack = audioCollector.createAudioTrack();
                            audioTrack.write(playAudioBuf, 0, playAudioBuf.length);
                            playAudioPosition = 0;
                        }
                    }

                    if (audioCollector.mOnG711AudioDataListener != null) {
                        int encodeLen = G711.encode(buffer, 0, bufferReadResult, encodeBuf);
                        audioCollector.mOnG711AudioDataListener.onG711AudioData(encodeBuf, encodeLen);
                    }
                }
            }

            audioCollector.releaseAudioRecord();
            audioCollector.releaseAudioTrack();
            audioCollector.mPlayAudio = false;
        }
    }

    public interface OnG711AudioDataListener {
        void onG711AudioData(byte[] audioData, int audioLen);
    }
}
