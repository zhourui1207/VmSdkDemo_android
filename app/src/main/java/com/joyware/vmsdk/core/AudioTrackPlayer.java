package com.joyware.vmsdk.core;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.support.annotation.NonNull;
import android.util.Log;

import com.joyware.vmsdk.util.G711;
import com.joyware.vmsdk.util.StringUtil;

import java.lang.ref.WeakReference;

/**
 * Created by zhourui on 17/6/10.
 * email: 332867864@qq.com
 * phone: 17006421542
 */

public class AudioTrackPlayer {

    public enum AudioType {
        G711A,
        G711U,
        PCM_8KHZ_16BIT_MONO
    }

    private final String TAG = "AudioTrackPlayer";

    private boolean mPlaying;
    private PlayThread mPlayThread;
    private long mRecNumber;

    public boolean isPlaying() {
        synchronized (this) {
            return mPlaying;
        }
    }

    // 开启播放线程
    public boolean start(int bufferMaxSize, AudioType audioType) {
        synchronized (this) {
            if (mPlaying) {
                Log.e(TAG, "Audio track player has been running already!");
                return false;
            }


            if (mPlayThread == null) {
                mPlayThread = new PlayThread(new WeakReference<AudioTrackPlayer>(this),
                        bufferMaxSize, audioType);
                mPlayThread.start();
            }

            mPlaying = true;
            return true;
        }
    }

    public boolean write(byte[] buf, int start, int len, int seqNumber) {
        synchronized (this) {
            if (!mPlaying) {
                Log.e(TAG, "Audio track player is't running!");
                return false;
            }

            return mPlayThread != null && mPlayThread.mBlockingBuffer.addObject(new PlayData(buf,
                    start, len, seqNumber, ++mRecNumber));
        }
    }

    public void clear() {
        synchronized (this) {
            if (!mPlaying) {
                Log.e(TAG, "Audio track player is't running!");
                return;
            }

            mRecNumber = 0;
            if (mPlayThread != null) {
                mPlayThread.mBlockingBuffer.clear();
            }
        }
    }

    public boolean stop() {
        synchronized (this) {
            if (!mPlaying) {
                Log.e(TAG, "Audio track player is't running!");
                return false;
            }

            if (mPlayThread != null) {
                mPlayThread.interrupt();
                try {
                    mPlayThread.join();
                } catch (InterruptedException e) {
                    Log.w(TAG, StringUtil.getStackTraceAsString(e));
                }

                mPlayThread = null;
            }

            mRecNumber = 0;
            mPlaying = false;
            return true;
        }
    }

    private static class PlayThread extends Thread {
        private String TAG = "AudioTrackPlayThread";

        @NonNull
        private AudioTrack mAudioTrack;

        @NonNull
        private WeakReference<AudioTrackPlayer> mAudioTrackPlayerWeakReference;

        @NonNull
        private BlockingBuffer mBlockingBuffer;

        private byte[] mDecodeBuffer;  // PCM数据

        private AudioType mAudioType;  // 音频类型

        private PlayThread(@NonNull WeakReference<AudioTrackPlayer>
                                   audioTrackPlayerWeakReference, int bufferMaxSize, AudioType
                                   audioType) throws IllegalArgumentException {

            mAudioType = audioType;

            int sampleRate = 0;
            int channelConfig = 0;
            int audioFormat = 0;
            switch (mAudioType) {
                case G711A:
                    sampleRate = 8000;
                    channelConfig = AudioFormat.CHANNEL_CONFIGURATION_MONO;
                    audioFormat = AudioFormat.ENCODING_PCM_16BIT;
                    break;
                case PCM_8KHZ_16BIT_MONO:
                    sampleRate = 8000;
                    channelConfig = AudioFormat.CHANNEL_CONFIGURATION_MONO;
                    audioFormat = AudioFormat.ENCODING_PCM_16BIT;
                    break;
//                case G711U:
//                    sampleRate = 8000;
//                    channelConfig = AudioFormat.CHANNEL_CONFIGURATION_MONO;
//                    audioFormat = AudioFormat.ENCODING_PCM_16BIT;
//                    break;
                default:
                    break;
            }

            if (sampleRate > 0 && channelConfig > 0 && audioFormat > 0) {
                mAudioTrackPlayerWeakReference = audioTrackPlayerWeakReference;
                mBlockingBuffer = new BlockingBuffer(BlockingBuffer.BlockingBufferType.PRIORITY,
                        bufferMaxSize);

                int minBufferSize = AudioTrack.getMinBufferSize(sampleRate, channelConfig,
                        audioFormat);
                mAudioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, sampleRate,
                        channelConfig, audioFormat, minBufferSize, AudioTrack.MODE_STREAM);
                mAudioTrack.play();
            } else {
                Log.e(TAG, "the audio type:" + audioType + " unsupported!");
                throw new IllegalArgumentException("the audio type:" + audioType + " unsupported!");
            }
        }

        @Override
        public void run() {

            try {
                while (!isInterrupted()) {
                    AudioTrackPlayer audioTrackPlayer = mAudioTrackPlayerWeakReference.get();
                    if (audioTrackPlayer == null) {
                        return;
                    }

                    PlayData playData = (PlayData) mBlockingBuffer.removeObjectBlocking();

                    if (playData == null) {
                        return;
                    }

                    byte[] tmpBuf = null;
                    int len = playData.getLen();

                    switch (mAudioType) {
                        case G711A:
                            if ((mDecodeBuffer == null) || (mDecodeBuffer.length != 2 * len)) {
                                mDecodeBuffer = new byte[2 * len];
                            }
                            int decodeLen = G711.decode(playData.getBuf(), 0, playData.getLen(),
                                    mDecodeBuffer);
                            if (decodeLen > 0) {
                                tmpBuf = mDecodeBuffer;
                                len = decodeLen;
                            }
                            break;
                        case PCM_8KHZ_16BIT_MONO:
                            tmpBuf = playData.getBuf();
                            break;
                        default:
                            break;
                    }

                    if (tmpBuf != null && len > 0) {
                        mAudioTrack.write(tmpBuf, 0, len);
                    }
                }
            } catch (InterruptedException e) {
                Log.d(TAG, StringUtil.getStackTraceAsString(e));
            }

            mAudioTrack.stop();
            mAudioTrack.release();
        }
    }

    private static class PlayData implements Comparable<PlayData> {
        private byte[] mBuf;
        private int mLen;
        private int mSeqNumber;
        private long mRecNumber;

        private PlayData(byte[] buf, int start, int len, int seqNumber, long recNumber) {
            if (len > 0) {
                mBuf = new byte[len];
                System.arraycopy(buf, start, mBuf, 0, len);
            }
            mLen = len;
            mSeqNumber = seqNumber;
            mRecNumber = recNumber;
        }

        public byte[] getBuf() {
            return mBuf;
        }

        public int getLen() {
            return mLen;
        }

        @Override
        public int compareTo(@NonNull PlayData o) {
            int compare =  ((Integer) mSeqNumber).compareTo(o.mSeqNumber);
            if (compare == 0) {
                compare = ((Long) mRecNumber).compareTo(o.mRecNumber);
            }
            return compare;
        }
    }
}
