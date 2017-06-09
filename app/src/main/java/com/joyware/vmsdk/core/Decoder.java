package com.joyware.vmsdk.core;

import android.graphics.Bitmap;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.os.AsyncTask;
import android.os.Build;
import android.support.annotation.NonNull;
import android.util.Log;

import com.joyware.vmsdk.ScreenshotCallback;
import com.joyware.vmsdk.VmType;
import com.joyware.vmsdk.util.G711;
import com.joyware.vmsdk.util.Mp4Save;
import com.joyware.vmsdk.util.PsStreamFilterUtil;
import com.joyware.vmsdk.util.StringUtil;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

/**
 * Created by zhourui on 16/10/28.
 * 解码器，包含软硬解视频，截图，实时录像，打开关闭音频等功能
 */

public class Decoder {

    private final int PAYLOAD_TYPE_PS = 96;

    private final String TAG = "Decoder";
    private final int BUFFER_MAX_SIZE = 500;
    private final int BUFFER_WARNING_SIZE = 400;

    // es数据类型
    private final int DATA_TYPE_UNKNOW = 0;  // 未知类型

    private final int DATA_TYPE_VIDEO_SPS = 1;   // sps数据
    private final int DATA_TYPE_VIDEO_PPS = 2;  // pps数据
    private final int DATA_TYPE_VIDEO_IFRAME = 3;  // I帧数据
    private final int DATA_TYPE_VIDEO_PFRAME = 4;  // P帧数据
    private final int DATA_TYPE_VIDEO_OTHER = 5;  // 其他数据

    private final int DATA_TYPE_AUDIO = 11;  // 音频数据

    private int decodeType = VmType.DECODE_TYPE_INTELL;

    private boolean openAudio = false;

    private volatile boolean isRunning = false;

    private HandleFrameThread handleFrameThread;
    private PlayThread playThread;
    private RecordThread recordThread;

    // 解码完成后的yuv数据回调
    private OnYUVFrameDataCallback mOnYUVFrameDataCallback;

    // 去掉ps头和拼帧后的裸码流数据回调
    private OnESFrameDataCallback mOnESFrameDataCallback;

    // 硬件解码相关
    private MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();

    // 高通骁龙820的时候，如果宽高填写1080*720的话，解码 325*288就会出现花屏，这个比较奇怪，所以我觉得理论上应该要先获取到图像宽高，然后再初始化mediacodec
    // 才是正确地做法
    private int w = 325;
    private int h = 288;
    private String mime = "video/avc";
    private MediaCodec mediaCodecDecoder;  //硬解码器
    private ByteBuffer[] inputBuffers;
    private ByteBuffer[] outputBuffers;

    private boolean mRealMode;
    private float mSpeedScale = 1.0f;

    private long firstPts;
    private long firstTimestamp;

    private boolean mNeedDecode = true;

    public void setSeepScale(float speedScale) {
        mSpeedScale = speedScale;
        firstTimestamp = 0;
    }

    public float getSpeedScale() {
        return mSpeedScale;
    }

    public interface OnYUVFrameDataCallback {
        void onFrameData(int width, int height, byte[] yData, int yStart, int yLen, byte[] uData,
                         int uStart, int uLen, byte[] vData, int vStart, int vLen);
    }

    public interface OnESFrameDataCallback {
        void onFrameData(boolean video, int timestamp, long pts, byte[] data, int start, int len);
    }

    public void setOnYUVFrameDataCallback(OnYUVFrameDataCallback onYUVFrameDataCallback) {
        mOnYUVFrameDataCallback = onYUVFrameDataCallback;
    }

    public void setOnESFrameDataCallback(OnESFrameDataCallback onESFrameDataCallback) {
        mOnESFrameDataCallback = onESFrameDataCallback;
    }

    private Decoder() {

    }

    public Decoder(int decodeType) {
        this.decodeType = decodeType;
    }

    public Decoder(int decodeType, boolean needDecode, boolean openAudio, boolean realMode) {
        Log.i(TAG, "构造解码器 decodeType=" + decodeType + ", needDecode=" + needDecode + ", realMode=" +
                realMode);
        this.decodeType = decodeType;
        this.openAudio = openAudio;
        mNeedDecode = needDecode;
        mRealMode = realMode;

        startPlay();
    }

    public static native long DecoderInit(int payloadType);

    public static native void DecoderUninit(long decoderHandle);

    public static native int DecodeNalu2RGB(long decoderHandle, byte[] inData, int inLen, byte[]
            outData, FrameConfHolder frameConfHolder);

    public static native int DecodeNalu2YUV(long decoderHandle, byte[] inData, int inLen, byte[]
            yData, byte[] uData, byte[] vData, FrameConfHolder frameConfHolder);

    public static native int GetFrameWidth(long decoderHandle);

    public static native int GetFrameHeight(long decoderHandle);

    public static native long AACEncoderInit(int samplerate);

    public static native void AACEncoderUninit(long encoderHandle);

    public static native int AACEncodePCM2AAC(long encoderHandle, byte[] inData, int inLen, byte[]
            outData);

    public static native boolean YUVSP2YUVP(byte[] inData, int inStart, int inLen, byte[] outData);

    public boolean addBuffer(int streamId, int streamType, int payloadType, byte[] buffer, int
            start, int len,
                             int timeStamp, int seqNumber, boolean isMark) {
        if (handleFrameThread != null) {
            return handleFrameThread.addBuffer(new StreamData(streamId, streamType, payloadType,
                    buffer, start, len, timeStamp, seqNumber, isMark));
        }
        return false;
    }

    public int getWidth() {
        if (playThread != null) {
            return w;
        }
        return 0;
    }

    public int getHeight() {
        if (playThread != null) {
            return h;
        }
        return 0;
    }

    public int getFramerate() {
        if (playThread != null) {
            return playThread.framerate;
        }
        return 25;
    }

    public synchronized void openAudio() {
        openAudio = true;
    }

    public synchronized void closeAudio() {
        openAudio = false;
    }

    public synchronized boolean isPlaying() {
        return isRunning;
    }

    public synchronized boolean startPlay() {
        if (handleFrameThread != null || playThread != null) {
            return true;
        }
        isRunning = true;
        try {
            if (handleFrameThread == null) {
                handleFrameThread = new HandleFrameThread();
                handleFrameThread.start();
            }
            if (playThread == null && mNeedDecode) {
                playThread = new PlayThread(decodeType);
                playThread.start();
            }
        } catch (Exception e) {
            return false;
        }
        return true;
    }

    public synchronized void stopPlay() {
//        if (handleFrameThread == null && playThread == null) {
//            return;
//        }

        isRunning = false;

        if (handleFrameThread != null) {
            handleFrameThread.shutDown();
            handleFrameThread = null;
        }

        if (recordThread != null) {
            recordThread.shutDown();
            recordThread = null;
        }

        if (playThread != null) {
            playThread.shutDown();
            playThread = null;
        }
    }

    public synchronized boolean isRecording() {
        return recordThread != null;
    }

    public synchronized boolean startRecord(String fileName) {
        if (recordThread != null && handleFrameThread != null) {
            return true;
        }

        isRunning = true;
        try {
            if (handleFrameThread == null) {
                handleFrameThread = new HandleFrameThread();
                handleFrameThread.start();
            }
            if (recordThread == null) {
                recordThread = new RecordThread(fileName);
                recordThread.start();
            }
        } catch (Exception e) {
            return false;
        }
        return true;
    }

    public synchronized void stopRecord() {
        if (handleFrameThread == null && recordThread == null) {
            return;
        }

        if (handleFrameThread != null && playThread == null) {
            isRunning = false;
            handleFrameThread.shutDown();
            handleFrameThread = null;
        }

        if (recordThread != null) {
            recordThread.shutDown();
            recordThread = null;
        }
    }

    public synchronized Bitmap screenshot(String fileName, ScreenshotCallback callback) {
//        if (recordThread != null && handleFrameThread != null) {  // 在录像的时候不能？
//            return null;
//        }

        if (playThread == null) {
            return null;
        }

        return playThread.screenshot(fileName, callback);
    }

    public synchronized void pauseDisplay() {
        if (playThread != null) {
            playThread.pauseDisplay();
        }
    }

    public synchronized void continueDisplay() {
        if (playThread != null) {
            playThread.continueDisplay();
        }
    }

    // 处理帧数据线程
    private class HandleFrameThread extends Thread {
        // 既有音频又有视频是为了兼容混合流
        byte[] videoBuf = new byte[409800]; // 400k  es元数据缓存，psp，pps，i帧，p帧，音频等
        int videoBufUsed;
        int streamBufUsed = 0;
        // 最新的4个字节内容
        int trans = 0xFFFFFFFF;
        // 保存包信息
        int payloadTypeOld;
        int header;  // 包头

        //        PsStreamUtil psStreamUtil = new PsStreamUtil();
        @NonNull
        PsStreamFilterUtil mPsStreamUtil = new PsStreamFilterUtil();

        int timestampOld;
        private BlockingBuffer streamBuffer = new BlockingBuffer(BUFFER_MAX_SIZE,
                BUFFER_WARNING_SIZE);  // 码流数据

        public boolean addBuffer(Object object) {
            return streamBuffer.addObjectForce(object);
        }

        public void shutDown() {
            this.interrupt();
            try {
                this.join();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }

        @Override
        public void run() {
            Log.i(TAG, "帧数据处理线程开始...");

            // 是否是第一帧
            final boolean[] isFirst = {true};
            boolean reset = true;

            final byte[][] remainingData = new byte[1][1];
            final int[] remainingStart = new int[1];
            final int[] remainingLen = new int[1];

            try {
                while (isRunning && !isInterrupted()) {
                    final StreamData streamData = (StreamData) streamBuffer.removeObjectBlocking();
                    if (!isRunning || streamData == null) {
                        break;
                    }
//                    if ((playThread == null && recordThread == null)) {
//                        continue;
//                    }

                    int streamType = streamData.getStreamType();

                    // 需要发送的数据
                    final int[] dataType = {DATA_TYPE_VIDEO_PFRAME};
                    byte[] data;
                    final int[] begin = {0};
                    int len;
                    final int payloadType = streamData.getPayloadType();
//                    Log.e(TAG, "payloadType=" + payloadType);
                    final int timestamp = streamData.getTimeStamp();

                    if (reset) {
                        resetVideoBuffer(payloadType, timestamp);
                        reset = false;
                    }

                    if (streamType == VmType.STREAM_TYPE_AUDIO) {  // 如果是音频，则不需要拼包和分包
                        dataType[0] = DATA_TYPE_AUDIO;
                        data = streamData.getBuffer();
                        begin[0] = 0;
                        len = streamData.getBuffer().length;

                        sendData(dataType[0], payloadType, timestamp, 0, data, begin[0], len);
                    } else if (streamType == VmType.STREAM_TYPE_AUDIO_G711A) {
                        if (!isFirst[0]) {  // 发现第一帧之后再发送音频
                            len = streamData.getBuffer().length;
                            byte[] dataForDecode = new byte[len * 2];
                            int audioLen = G711.decode(streamData.getBuffer(), begin[0],
                                    len, dataForDecode);

                            dataType[0] = DATA_TYPE_AUDIO;
                            sendData(dataType[0], payloadType, timestamp, 0, dataForDecode, 0,
                                    audioLen);
                        }
                    } else if (streamType == VmType.STREAM_TYPE_VIDEO) {  // 视频数据
                        // 这里需要判断是否是PS流，如果是复合的PS流，还需要再分出视频和音频流
                        boolean isVideo = true;
                        if (payloadTypeOld == PAYLOAD_TYPE_PS) {
//                            begin = psStreamUtil.filterPsHeader(streamData.getBuffer(), 0,
//                                    streamData.getBuffer().length);
//                            if (begin < streamData.getBuffer().length) {  // 表示有数据可以使用
//                                if (!psStreamUtil.isFindDataStart()) {
//                                    continue;
//                                }
//
//                                if (psStreamUtil.isVideo()) {
//                                    isVideo = true;
//                                } else {  // 是音频
//                                    isVideo = false;
//
//                                    if (!isFirst) {  // 发现第一帧之后再发送音频
//                                        int lenNow = streamData.getBuffer().length - begin;
//                                        byte[] dataForDecode = new byte[lenNow * 2];
//                                        int audioLen = G711.decode(streamData.getBuffer(), begin,
//                                                lenNow, dataForDecode);
//
//                                        dataType = DATA_TYPE_AUDIO;
//                                        sendData(dataType, payloadType, timestamp,
// dataForDecode, 0,
//                                                audioLen);
//                                    }
//                                }
//                            } else {
//                            }

                            data = streamData.getBuffer();
                            len = data.length;
                            begin[0] = 0;

                            if (remainingData[0] != null && remainingLen[0] > 0) {
                                int totalLen = remainingLen[0] + len;
                                byte[] totalData = new byte[totalLen];
                                System.arraycopy(remainingData[0], remainingStart[0], totalData,
                                        0, remainingLen[0]);
                                System.arraycopy(data, 0, totalData, remainingLen[0], len);
                                data = totalData;
                                len = totalLen;

                                remainingData[0] = null;
                                remainingLen[0] = 0;
                            }

//                            Log.e(TAG, "filterPsHeader len=" + len + ", start=" + begin[0] + " " +
//                                    "mData=" + StringUtil.byte2hex(mData, 0, len));
                            mPsStreamUtil.filterPsHeader(data, begin[0], len, new
                                    PsStreamFilterUtil.OnDataCallback() {

                                        @Override
                                        public void onESData(boolean video, long pts, byte[]
                                                outData, int outStart, int outLen) {
//                                            Log.e(TAG, "mVideo=" + mVideo + ", pts=" + pts);
//                                            Log.e(TAG, "onESData video=" + video + ", start=" +
//                                                    outStart + ", len=" + outLen + ", mData=" +
//                                                    StringUtil.byte2hex(outData, outStart,
// outLen));
                                            if (video) {
                                                streamBufUsed = 0;

                                                while (outLen - streamBufUsed > 0) {
                                                    // 分帧
                                                    int nalLen = mergeBuffer(videoBuf, videoBufUsed,
                                                            outData, outStart + streamBufUsed,
                                                            outLen - streamBufUsed);
                                                    videoBufUsed += nalLen;
                                                    streamBufUsed += nalLen;

                                                    // 分出完整帧
                                                    if (trans == 1) {
//                                                        Log.e(TAG, "完整帧");
                                                        // 第一帧的数据可能只会有部分内容，通常将它抛弃比较合理
                                                        if (isFirst[0] && (videoBuf[4] & 0x1F) !=
                                                                0x07) {
                                                            //
                                                            // 一开始如果不是I帧，就全部丢弃，加快显示速度，还能不用看到绿屏
                                                            // 发送完之后再重置视频缓存
                                                            resetVideoBuffer(payloadType,
                                                                    timestamp);
                                                        } else {
//                                                            Log.e(TAG, "找到I帧");
                                                            isFirst[0] = false;
                                                            boolean send = true;
//                                        Log.e(TAG, "videoBuf[4]=" + videoBuf[4]);
                                                            if ((videoBuf[4] & 0x1F) == 0x07) {
                                                                // sps
                                                                dataType[0] = DATA_TYPE_VIDEO_SPS;
                                                            } else if ((videoBuf[4] & 0x1F) ==
                                                                    0x08) {
                                                                // pps
                                                                dataType[0] = DATA_TYPE_VIDEO_PPS;
                                                            } else if ((videoBuf[4] & 0x1F) ==
                                                                    0x05) {
                                                                // I帧
                                                                dataType[0] =
                                                                        DATA_TYPE_VIDEO_IFRAME;
                                                            } else if ((videoBuf[4] & 0x1F) ==
                                                                    0x06) {
                                                                dataType[0] = DATA_TYPE_VIDEO_OTHER;
                                                            } else {  // 否则都当做P帧
                                                                dataType[0] =
                                                                        DATA_TYPE_VIDEO_PFRAME;
                                                            }
//                                        Log.e(TAG, "videoBuf[4]=" + videoBuf[4]);

//                                        Log.e(TAG, "一帧数据");
                                                            if (send) {
                                                                sendData(dataType[0], payloadType,
                                                                        timestamp, pts, videoBuf,
                                                                        0, videoBufUsed - 4);
                                                            }
                                                        }
                                                    }
                                                }
                                            } else {
                                                if (!isFirst[0]) {  // 发现第一帧之后再发送音频
                                                    byte[] dataForDecode = new byte[outLen * 2];
                                                    int audioLen = G711.decode(outData, outStart,
                                                            outLen,
                                                            dataForDecode);

                                                    dataType[0] = DATA_TYPE_AUDIO;
                                                    sendData(dataType[0], payloadType, timestamp,
                                                            pts, dataForDecode, 0, audioLen);
                                                }
                                            }
                                        }

                                        @Override
                                        public void onRemainingData(byte[] outData, int outStart,
                                                                    int outLen) {
//                                            Log.e(TAG, "onRemainingData start=" +
//                                                    outStart + ", len=" + outLen + ", mData=" +
//                                                    StringUtil.byte2hex(outData, outStart,
// outLen));
                                            remainingData[0] = outData;
                                            remainingStart[0] = outStart;
                                            remainingLen[0] = outLen;
                                        }
                                    });

                            continue;
                        }

                        if (isVideo) {
                            int packetLen = streamData.getBuffer().length;
                            streamBufUsed = begin[0];

                            while (packetLen - streamBufUsed > 0) {
                                // 分帧
                                int nalLen = mergeBuffer(videoBuf, videoBufUsed, streamData
                                                .getBuffer(),
                                        streamBufUsed, packetLen - streamBufUsed);
                                videoBufUsed += nalLen;
                                streamBufUsed += nalLen;

                                // 分出完整帧
                                if (trans == 1) {
                                    // 第一帧的数据可能只会有部分内容，通常将它抛弃比较合理
                                    if (isFirst[0] && (videoBuf[4] & 0x1F) != 0x07) {  //
                                        // 一开始如果不是I帧，就全部丢弃，加快显示速度，还能不用看到绿屏
                                        // 发送完之后再重置视频缓存
                                        resetVideoBuffer(payloadType, timestamp);
                                    } else {
                                        isFirst[0] = false;
                                        boolean send = true;
//                                        Log.e(TAG, "videoBuf[4]=" + videoBuf[4]);
                                        if ((videoBuf[4] & 0x1F) == 0x07) {  // sps
                                            dataType[0] = DATA_TYPE_VIDEO_SPS;
                                        } else if ((videoBuf[4] & 0x1F) == 0x08) {  // pps
                                            dataType[0] = DATA_TYPE_VIDEO_PPS;
                                        } else if ((videoBuf[4] & 0x1F) == 0x05) {  // I帧
                                            dataType[0] = DATA_TYPE_VIDEO_IFRAME;
                                        } else if ((videoBuf[4] & 0x1F) == 0x06) {
                                            dataType[0] = DATA_TYPE_VIDEO_OTHER;
                                        } else {  // 否则都当做P帧
                                            dataType[0] = DATA_TYPE_VIDEO_PFRAME;
                                        }
//                                        Log.e(TAG, "videoBuf[4]=" + videoBuf[4]);
                                        data = videoBuf;
                                        begin[0] = 0;
                                        len = videoBufUsed - 4;  // 最后会多4个字节的0x00 00 00 01

//                                        Log.e(TAG, "一帧数据");
                                        if (send) {
                                            sendData(dataType[0], payloadType, timestamp, 0, data,
                                                    begin[0], len);
                                        }
                                    }
                                }
                            }
                        }

                    } else {
                        Log.e(TAG, "暂时不支持[" + streamType + "]类型码流的解码！");
                    }

                }
            } catch (InterruptedException e) {

            }
            Log.w(TAG, "帧数据处理线程结束...");
        }

//        private int mOrderBufferSize = 25;  // 排序缓存大小
//        @NonNull
//        private LinkedList<EsStreamData> mVideoOrderBuffer = new LinkedList<>();
//        @NonNull
//        private LinkedList<EsStreamData> mAudioOrderBuffer = new LinkedList<>();

        private void sendData(int dataType, int payloadType, int timestamp, long pts, byte[]
                data, int begin,
                              int len) {

            // 需要发送的数据不为空时，需要发送
            if (data != null) {

//                if ()
//
//                EsStreamData esStreamData = new EsStreamData(dataType, payloadTypeOld,
//                        timestampOld, pts, data, begin, len);
//
//                // 这里应该对pts做一个简单地排序功能从小往大
//                if (dataType == DATA_TYPE_AUDIO) {
//                    mAudioOrderBuffer.add(esStreamData);
//                    Collections.sort(mAlarmNotifies);
//                } else {
//                    mVideoOrderBuffer.add(esStreamData);
//                }
//
//                if (mAudioOrderBuffer)
                // 解析出es元数据后，看线程是否需要使用该数据，发送数据到相应的线程
                if (playThread != null) {
                    EsStreamData esStreamData = new EsStreamData(dataType, payloadTypeOld,
                            timestampOld, pts, data, begin, len);
                    playThread.addBuffer(esStreamData);
                }
                if (!mNeedDecode && mOnESFrameDataCallback != null) {  //
                    // 如果不解码的话，就是说不需要控制播放速度，那么在这里回调
                    mOnESFrameDataCallback.onFrameData(dataType != DATA_TYPE_AUDIO, timestamp,
                            pts, data, begin, len);
                }
                // 发送完之后再重置视频缓存
                if (dataType != DATA_TYPE_AUDIO) {
                    resetVideoBuffer(payloadType, timestamp);
                }
            }
        }

        private void resetVideoBuffer(int payloadType, int timestamp) {
            trans = 0xFFFFFFFF;

            payloadTypeOld = payloadType;
            timestampOld = timestamp;

//            if (payloadTypeOld == PAYLOAD_TYPE_PS) {  // 若是PS流
//                videoBuf[0] = 0x00;
//                videoBuf[1] = 0x00;
//                videoBuf[2] = 0x01;
//                videoBuf[3] = (byte) 0xBA;
//                header = 0x000001BA;
//            } else {
//                videoBuf[0] = 0x00;
//                videoBuf[1] = 0x00;
//                videoBuf[2] = 0x00;
//                videoBuf[3] = 0x01;
//                header = 0x00000001;
//            }
            videoBuf[0] = 0x00;
            videoBuf[1] = 0x00;
            videoBuf[2] = 0x00;
            videoBuf[3] = 0x01;
            header = 0x00000001;

            videoBufUsed = 4;
        }

        // 根据0x00 00 00 01 nalutype 来进行分段
        private int mergeBuffer(byte[] NalBuf, int NalBufUsed, byte[] SockBuf, int SockBufUsed,
                                int SockRemain) {
            int i = 0;

            if (true) {
                byte Temp;
                for (; i < SockRemain; ++i) {
                    Temp = SockBuf[i + SockBufUsed];
                    NalBuf[i + NalBufUsed] = Temp;

                    trans <<= 8;
                    trans |= Temp;

                    if (trans == header) {
                        ++i;
                        break;
                    }
                }
            } else {
                if (SockBuf[SockBufUsed] == 0x00 && SockBuf[SockBufUsed + 1] == 0x00 &&
                        SockBuf[SockBufUsed + 2] == 0x00 && SockBuf[SockBufUsed + 3] == 0x01) {
                    //System.arraycopy(SockBuf, SockBufUsed, NalBuf, NalBufUsed, 4);
                    //Log.e(TAG, "start");
                    i = 4;
                    trans = header;
                } else {
                    //Log.e(TAG, "continue");
                    System.arraycopy(SockBuf, SockBufUsed, NalBuf, NalBufUsed, SockRemain);
                    i = SockRemain;
                }

            }

            return i;
        }
    }

    // 播放线程
    private class PlayThread extends Thread {
        private int decodeType;


        private long mVideoBeginTime;
        private long mAudioBeginTime;
        // 帧率
        private int framerate = 25;
        // 是否获取到配置
        private boolean getConf = false;

        // 帧间隔 默认40毫秒
        private long interval = 40;

        private byte[] outData;
        private int mColorFormatType = 0;
        private byte[] mTmpVUBuffer;

        private int currentPayloadType;


        // 软件解码相关
        private long decoderHandle;
        private byte[] pixelBuffer;

        // 截图相关
        private byte[] lastSpsBuffer;
        private byte[] lastPpsBuffer;
        private byte[] lastIFrameBuffer;  // 最后一帧I帧数据
        private byte[] lastPFrameBuffer;  // I帧后紧跟着的P帧  测试发现有些设备在解码I帧后无法输出图像，必须输入后面的P帧才能显示

        private byte[] tmpIFrameBuffer;  // 临时的I帧，用来存储还没更新lastPFrameBuffer时的数据
        private boolean isFistPFrame = true;

        // oepngles渲染
        private boolean useOpengles = true;

        // 使用opengles时，从c++层返回的YUV数据
        private byte[] yBuffer;
        private byte[] uBuffer;
        private byte[] vBuffer;

        private FrameConfHolder frameConfHolder = new FrameConfHolder();

        private boolean showed = false;

//        private Display displayThread;

        private PlayAudio playAudioThread;

        private long timestamp;

        // 播放数据
        private BlockingBuffer playBuffer = new BlockingBuffer(mRealMode ? BUFFER_MAX_SIZE :
                BUFFER_MAX_SIZE * 1000,
                mRealMode ? BUFFER_WARNING_SIZE : BUFFER_WARNING_SIZE * 1000);

        // surface是否被创建
        private boolean surfaceCreated = false;
        private boolean isPause = false;

        OutputThread outputThread;

        private boolean useAudioTimestamp;

        public PlayThread(int decodeType) {
            this.decodeType = decodeType;
        }

        public synchronized Bitmap screenshot(String fileName, final ScreenshotCallback callback) {
            Log.w(TAG, "screenshot fileName=" + fileName);
            if (callback != null) {
                new ScreenshotTask(fileName, callback).executeOnExecutor(AsyncTask
                        .THREAD_POOL_EXECUTOR);
            } else {
                return screenshotAndSave(fileName);
            }

            return null;
        }

        private Bitmap screenshotAndSave(String fileName) {
            Bitmap screenBit = null;

            if (lastSpsBuffer == null || lastPpsBuffer == null || lastIFrameBuffer == null) {
                Log.e(TAG, "screenshot failed， buffr error! lastSpsBuffer=" + lastSpsBuffer + ", " +
                        "lastPpsBuffer=" + lastPpsBuffer + ", lastIFrameBuffer=" +
                        lastIFrameBuffer);
                return screenBit;
            }

            FileOutputStream fileOutputStream = null;
            if (fileName != null && !fileName.isEmpty()) {
                try {
                    fileOutputStream = new FileOutputStream(fileName);
                } catch (FileNotFoundException e) {
                    Log.e(TAG, StringUtil.getStackTraceAsString(e));
                    return screenBit;
                }
            }

            int size = w * h * 2;
            byte[] rgbData = new byte[size];

//      Log.e(TAG, "size=" + size);

            FrameConfHolder frameConfHolder = new FrameConfHolder();
            long screenshotDecoderHandle = DecoderInit(currentPayloadType);
            DecodeNalu2RGB(screenshotDecoderHandle, lastSpsBuffer, lastSpsBuffer.length, rgbData,
                    frameConfHolder);
            DecodeNalu2RGB(screenshotDecoderHandle, lastPpsBuffer, lastPpsBuffer.length, rgbData,
                    frameConfHolder);
            int ret = DecodeNalu2RGB(screenshotDecoderHandle, lastIFrameBuffer, lastIFrameBuffer
                    .length, rgbData, frameConfHolder);
            if (ret <= 0) {
                ret = DecodeNalu2RGB(screenshotDecoderHandle, lastPFrameBuffer, lastPFrameBuffer
                                .length,
                        rgbData, frameConfHolder);
            }
            DecoderUninit(screenshotDecoderHandle);

//      Log.e(TAG, "ret=" + ret);
            if (ret <= 0) {
                Log.e(TAG, "screenshot decode failed ret=" + ret);
                return screenBit;
            }


            try {
                ByteBuffer buffer = ByteBuffer.wrap(rgbData, 0, size);
                screenBit = Bitmap.createBitmap(w, h, Bitmap.Config.RGB_565);
                buffer.rewind();
                screenBit.copyPixelsFromBuffer(buffer);

                if (fileOutputStream != null) {
                    screenBit.compress(Bitmap.CompressFormat.JPEG, 100, fileOutputStream);
                    fileOutputStream.flush();
                }

            } catch (Exception e) {
                Log.e(TAG, StringUtil.getStackTraceAsString(e));
                return screenBit;
            } finally {
                if (fileOutputStream != null) {
                    try {
                        fileOutputStream.close();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
            }
            return screenBit;
        }

        private class ScreenshotTask extends AsyncTask<Void, Void, Bitmap> {
            private ScreenshotCallback screenshotCallback;
            private String fileName;

            private ScreenshotTask() {
            }

            public ScreenshotTask(String fileName, ScreenshotCallback screenshotCallback) {
                this.screenshotCallback = screenshotCallback;
                this.fileName = fileName;
            }

            @Override
            protected Bitmap doInBackground(Void... params) {
                if (this.fileName == null || this.fileName.isEmpty()) {
                    return null;
                }

                return screenshotAndSave(fileName);
            }

            @Override
            protected void onPostExecute(Bitmap bitmap) {
                if (this.screenshotCallback != null) {
                    if (bitmap != null) {
                        this.screenshotCallback.onSuccess(this.fileName, bitmap);
                    } else {
                        this.screenshotCallback.onFailed("");
                    }
                }
            }
        }

        private void createAudioThread() {
            if (playAudioThread == null) {
                playAudioThread = new PlayAudio();
                playAudioThread.start();
            }
        }

        private void releaseAudioThread() {
            if (playAudioThread != null) {
                playAudioThread.shutDown();
                playAudioThread = null;
            }
        }

        private void createDecoder(int payloadType) {
            if (isRunning) {
                currentPayloadType = payloadType;

                if (decodeType == VmType.DECODE_TYPE_SOFTWARE) {
                    if (decoderHandle == 0) {
                        if (useOpengles) {  // 不使用opengles渲染
                            int ySize = w * h;
                            int uvSize = w * h / 4;
                            yBuffer = new byte[ySize];
                            uBuffer = new byte[uvSize];
                            vBuffer = new byte[uvSize];
                        } else {
                            int size = w * h * 2;
                            // pixelBuffer接收解码数据的缓存
                            pixelBuffer = new byte[size];
                        }

                        Log.w(TAG, "创建软件解码");
                        decoderHandle = DecoderInit(payloadType);
                        Log.i(TAG, "decoderHandle=" + decoderHandle);
                    }
                } else {
                    if (mediaCodecDecoder == null && isRunning) {
                        Log.w(TAG, "创建硬件解码");
                        try {
                            mediaCodecDecoder = MediaCodec.createDecoderByType(mime);
                            // 其实这个只有编码的时候才有用
//                            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
//                                mediaFormat.setInteger(KEY_COLOR_FORMAT,
//                                        MediaCodecInfo.CodecCapabilities
//                                                .COLOR_FormatYUV420Flexible);
//                            } else {
//                                mediaFormat.setInteger(KEY_COLOR_FORMAT,
//                                        COLOR_FormatYUV420Planar);
//                            }
                            MediaFormat mediaFormat = MediaFormat.createVideoFormat(mime, w, h);
                            mediaCodecDecoder.configure(mediaFormat, null, null, 0);  //
// todo:不传入surface，只用来解码，自己来显示

                            mediaCodecDecoder.start();


                            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) {
                                inputBuffers = mediaCodecDecoder.getInputBuffers();
                                outputBuffers = mediaCodecDecoder.getOutputBuffers();
                            }

                        } catch (Exception e) {
                            if (decodeType == VmType.DECODE_TYPE_INTELL) {
                                decodeType = VmType.DECODE_TYPE_SOFTWARE;
                            }
                            Log.e(TAG, StringUtil.getStackTraceAsString(e));
                        }
                    }
                    if (outputThread == null && isRunning) {
                        outputThread = new OutputThread();
                        outputThread.start();
                    }
                }
            }
        }

        private void releaseDecoder() {
            if (decoderHandle != 0) {
                Log.e(TAG, "DecoderUninit " + decoderHandle);
                DecoderUninit(decoderHandle);
                decoderHandle = 0;
            }

            if (outputThread != null) {
                outputThread.interrupt();
                try {
                    outputThread.join();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                outputThread = null;
            }

            if (mediaCodecDecoder != null) {
                try {
                    mediaCodecDecoder.stop();
                } catch (Exception e) {
                    Log.e(TAG, StringUtil.getStackTraceAsString(e));
                    // 如果这里还出错，证明不支持硬件解码，那么使用智能解码的话，就改用软解码了
                    // 还有一中出错方式是窗口已经不存在
                    if (decodeType == VmType.DECODE_TYPE_INTELL && surfaceCreated) {
                        decodeType = VmType.DECODE_TYPE_SOFTWARE;
                    }
                }
                try {
                    mediaCodecDecoder.release();
                } catch (Exception e) {
                    Log.e(TAG, StringUtil.getStackTraceAsString(e));
                }

                mediaCodecDecoder = null;
            }

            showed = false;
        }

        public boolean addBuffer(Object object) {
            return playBuffer.addObject(object);
        }

        public void shutDown() {
            this.interrupt();
            try {
                this.join();
            } catch (Exception e) {
                Log.e(TAG, StringUtil.getStackTraceAsString(e));
            }
//            if (displayThread != null) {
//                displayThread.shutDown();
//                displayThread = null;
//            }
            releaseAudioThread();
        }

        //    FileOutputStream fileOutputStream;

        @Override
        public void run() {
            Log.w(TAG, "解码线程开始...");

            try {
                while (isRunning && !this.isInterrupted()) {

                    if (isPause) {
                        firstPts = 0;
                        firstTimestamp = 0;
                        sleep(10);
                        continue;
                    }

                    EsStreamData esStreamData = (EsStreamData) playBuffer.removeObjectBlocking();
                    if (!isRunning || esStreamData == null) {
                        break;
                    }

                    if (recordThread != null) {
//                        EsStreamData esStreamData = new EsStreamData(dataType, payloadTypeOld,
//                                timestampOld, mData, begin, len);
                        recordThread.addBuffer(esStreamData);
                    }

                    int dataType = esStreamData.getDataType();

                    if (mOnESFrameDataCallback != null) {
                        mOnESFrameDataCallback.onFrameData(dataType != DATA_TYPE_AUDIO,
                                esStreamData.getTimestamp(), esStreamData.getPts(), esStreamData
                                        .getData(), 0, esStreamData.getData().length);
                    }

                    // 如果队列中大于100帧没有处理，则做丢帧处理，只对p帧丢包，别的包不可以丢
                    if (mRealMode && playBuffer.size() > 100 && (dataType ==
                            DATA_TYPE_VIDEO_PFRAME ||
                            dataType == DATA_TYPE_AUDIO)) {
                        // todo: 发现问题，如果夹带着有音频包并且需要播放音频，这一个线程吃不消，
                        // 一旦队列超过100后，丢失掉视频帧也无法将队列数量降下来，将这个放音频处理上面看看
                        Log.e(TAG, "buffer over 100, miss mData");
                        continue;
                    }

                    // 如果是音频的话，则只需要执行这段代码即可
                    if (dataType == DATA_TYPE_AUDIO) {
                        // 当音频和视频同时存在的时候，优先控制音频播放速度
                        if (!useAudioTimestamp) {
                            useAudioTimestamp = true;
                            firstTimestamp = 0;
                            firstPts = 0;
                        }
                        if (useAudioTimestamp && !mRealMode && esStreamData.getPts() != 0) {  //
                            // 不为实时模式并且pts不为0
                            if (firstTimestamp > 0) {
                                // 计算显示时间 纳秒
                                long showTime = (long) (1000000000f * (esStreamData.getPts() -
                                        firstPts) / 90000 / mSpeedScale) + firstTimestamp;
                                while (System.nanoTime() < showTime) {  // 如果当前时间小于显示时间，则等待
                                    sleep(0);
                                }
                            } else {
                                firstPts = esStreamData.getPts();
                                firstTimestamp = System.nanoTime();
                            }
                        }
                        if (openAudio) {
                            createAudioThread();
                            if (playAudioThread != null) {
                                playAudioThread.addBuffer(esStreamData);
                            }
                        } else {
                            releaseAudioThread();
                        }
                        continue;
                    }

//                    Log.e(TAG, "播放视频");
                    createDecoder(esStreamData.getPayloadType());

                    // 如果是没显示过，那么需要从sps开始解码
                    if (!showed && dataType != DATA_TYPE_VIDEO_SPS) {
                        continue;
                    }

                    showed = true;

                    if ((decodeType == VmType.DECODE_TYPE_INTELL || decodeType == VmType
                            .DECODE_TYPE_HARDWARE) && mediaCodecDecoder == null) {
                        continue;
                    }

                    if ((decodeType == VmType.DECODE_TYPE_SOFTWARE) && decoderHandle == 0) {
                        continue;
                    }

                    byte[] data = esStreamData.getData();

//                    Log.e(TAG, "type=" + (mData[4] & 0x1f));
                    // 保存最新的I帧和sps、pps数据供截图使用
                    if (dataType == DATA_TYPE_VIDEO_SPS) {
                        lastSpsBuffer = new byte[data.length];
                        System.arraycopy(data, 0, lastSpsBuffer, 0, data.length);
//                        Log.e(TAG, "sps");
                    } else if (dataType == DATA_TYPE_VIDEO_PPS) {
                        lastPpsBuffer = new byte[data.length];
                        System.arraycopy(data, 0, lastPpsBuffer, 0, data.length);
//                        Log.e(TAG, "pps");
                    } else if (dataType == DATA_TYPE_VIDEO_IFRAME) {
                        tmpIFrameBuffer = new byte[data.length];
                        System.arraycopy(data, 0, tmpIFrameBuffer, 0, data.length);
//                        lastIFrameBuffer = tmpIFrameBuffer;
//            lastIFrameBuffer = new byte[mData.length];
//            System.arraycopy(mData, 0, lastIFrameBuffer, 0, mData.length);
                        isFistPFrame = true;
//                        Log.e(TAG, "IFrame");
                    } else if (dataType == DATA_TYPE_VIDEO_PFRAME && isFistPFrame &&
                            tmpIFrameBuffer !=
                                    null) {
//                        Log.e(TAG, "PFrame");
                        lastIFrameBuffer = tmpIFrameBuffer;
                        lastPFrameBuffer = new byte[data.length];
                        System.arraycopy(data, 0, lastPFrameBuffer, 0, data.length);
                        isFistPFrame = false;
                    }

                    // 硬件解码的话需要使用软解码获取图像信息
                    decodeConf(esStreamData.getPayloadType());

                    if (decodeType == VmType.DECODE_TYPE_SOFTWARE) {  // 软件解码

                        int canShow;
                        canShow = DecodeNalu2YUV(decoderHandle, data, data.length, yBuffer,
                                uBuffer,
                                vBuffer, frameConfHolder);

//                            Log.e(TAG, "解码canShow=" + canShow);
                        if (canShow >= 0) {
                            int tmpWidth = frameConfHolder.getWidth() > 0 ? frameConfHolder
                                    .getWidth() : w;
                            int tmpHeight = frameConfHolder.getHeight() > 0 ? frameConfHolder
                                    .getHeight() : h;
                            int tmpFramerate = frameConfHolder.getFramerate();
                            if (tmpFramerate > 0 && framerate != tmpFramerate) {
                                Log.w(TAG, "码率改变 " + framerate + " --> " + tmpFramerate);
                                framerate = tmpFramerate;
                                interval = 1000 / framerate;
                            }
                            if (w != tmpWidth || h != tmpHeight) {
                                w = tmpWidth;
                                h = tmpHeight;
                                Log.w(TAG, "解码图像宽高发生变化,重新生成缓冲大小, w=" + w + ", h=" + h);

                                int ySize = w * h;
                                int uvSize = w * h / 4;
                                yBuffer = new byte[ySize];
                                uBuffer = new byte[uvSize];
                                vBuffer = new byte[uvSize];

                                // 重新解一次码
                                canShow = DecodeNalu2YUV(decoderHandle, data, data.length,
                                        yBuffer, uBuffer,
                                        vBuffer, frameConfHolder);
                            }
                            // 可以显示
                            if (canShow > 0 && mOnYUVFrameDataCallback != null) {
                                // 异步处理
                                mOnYUVFrameDataCallback.onFrameData(w, h, yBuffer, 0, yBuffer
                                                .length,
                                        uBuffer, 0, uBuffer.length, vBuffer, 0, vBuffer.length);
                            }
                        }
                    } else {  // 智能解码或者硬件解码
                        int inputBufferIndex;

                        try {

                            long timeout_us = 80000;
                            if (dataType == DATA_TYPE_VIDEO_IFRAME) {
                                timeout_us = 80000;
                            }
                            inputBufferIndex = mediaCodecDecoder.dequeueInputBuffer(timeout_us);

                            if (inputBufferIndex >= 0) {
                                ByteBuffer inputBuffer;
//                                inputBuffer = inputBuffers[inputBufferIndex];
//                                inputBuffer.clear();
                                if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) {
                                    inputBuffer = inputBuffers[inputBufferIndex];
                                    inputBuffer.clear();
                                } else {
                                    inputBuffer = mediaCodecDecoder.getInputBuffer
                                            (inputBufferIndex);
                                }

                                if (inputBuffer != null) {
                                    inputBuffer.put(data, 0, data.length);
                                    int flags;
                                    switch (dataType) {
                                        case DATA_TYPE_VIDEO_SPS:
                                        case DATA_TYPE_VIDEO_PPS:
                                            flags = MediaCodec.BUFFER_FLAG_CODEC_CONFIG;
                                            break;
                                        default:
                                            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES
                                                    .LOLLIPOP) {
                                                flags = MediaCodec.BUFFER_FLAG_KEY_FRAME;
                                            } else {
                                                flags = MediaCodec.BUFFER_FLAG_SYNC_FRAME;
                                            }
//                                            timestamp = (timestamp + 40000) % Long.MAX_VALUE;
                                            timestamp += 100;
                                            break;
                                    }
                                    mediaCodecDecoder.queueInputBuffer(inputBufferIndex, 0,
                                            data.length, timestamp, flags);
                                }
                            } else {
                                Log.e(TAG, "input failed mData=" + dataType);
                            }

                        } catch (Exception e) {
                            Log.w(TAG, StringUtil.getStackTraceAsString(e));
                            releaseDecoder();
                        }
                    }

                    if (!useAudioTimestamp && !mRealMode && esStreamData.getPts() != 0) {  //
                        // 不为实时模式并且pts不为0
                        if (firstTimestamp > 0) {
                            // 计算显示时间 纳秒
                            long showTime = (long) (1000000000f * (esStreamData.getPts() -
                                    firstPts) / 90000 / mSpeedScale) + firstTimestamp;
                            while (System.nanoTime() < showTime) {  // 如果当前时间小于显示时间，则等待
                                sleep(0);
                            }
                        } else {
                            firstPts = esStreamData.getPts();
                            firstTimestamp = System.nanoTime();
                        }
                    }
                }
            } catch (InterruptedException e) {
                Log.w(TAG, "解码线程中断!");
                Log.w(TAG, StringUtil.getStackTraceAsString(e));
            }
            Log.w(TAG, "释放解码器");
            releaseDecoder();
            Log.w(TAG, "解码线程结束...");
        }

        // 获取视频信息
        private void decodeConf(int payloadType) {
            if (decodeType != VmType.DECODE_TYPE_SOFTWARE && !getConf && lastIFrameBuffer != null &&
                    lastSpsBuffer != null && lastPpsBuffer != null) {
                FrameConfHolder frameConfHolder = new FrameConfHolder();
                long decodeConfDecoderHandle = DecoderInit(payloadType);
                DecodeNalu2RGB(decodeConfDecoderHandle, lastSpsBuffer, lastSpsBuffer.length, null,
                        frameConfHolder);
                DecodeNalu2RGB(decodeConfDecoderHandle, lastPpsBuffer, lastPpsBuffer.length, null,
                        frameConfHolder);
                int ret = DecodeNalu2RGB(decodeConfDecoderHandle, lastIFrameBuffer, lastIFrameBuffer
                        .length, null, frameConfHolder);
                if (ret <= 0) {
                    ret = DecodeNalu2RGB(decodeConfDecoderHandle, lastPFrameBuffer, lastPFrameBuffer
                            .length, null, frameConfHolder);
                }

                DecoderUninit(decodeConfDecoderHandle);

                if (ret > 0) {
                    w = frameConfHolder.getWidth();
                    h = frameConfHolder.getHeight();
                    framerate = frameConfHolder.getFramerate();
                    Log.w(TAG, "获取视频信息成功! w=" + w + ", h=" + h + ", framerate=" + framerate);
                    getConf = true;
                }
            }
        }


        public void pauseDisplay() {
            isPause = true;
        }

        public void continueDisplay() {
            isPause = false;
        }

//        public void openAudio() {
//            openAudio = true;
//        }
//
//        public void closeAudio() {
//            openAudio = false;
//        }

        private class PlayAudio extends Thread {
            // 解码出来后的数据队列
            private BlockingBuffer audioBuffer = new BlockingBuffer(100, 80);  //
            // 最多放延迟25帧，再多的话，内存会崩溃

            private AudioTrack audioDecoder;

            public PlayAudio() {

            }

            public boolean addBuffer(Object object) {
                return audioBuffer.addObjectForce(object);
            }

            public void shutDown() {
                this.interrupt();
                try {
                    this.join();
                } catch (InterruptedException e) {
                    Log.e(TAG, StringUtil.getStackTraceAsString(e));
                }
            }

            private void createAudioDecoder() {
                if (audioDecoder != null) {
                    return;
                }
                //音频解码器
                int samp_rate = 8000;
                int maxjitter = AudioTrack.getMinBufferSize(samp_rate,
                        AudioFormat.CHANNEL_CONFIGURATION_MONO,
                        AudioFormat.ENCODING_PCM_16BIT);
                audioDecoder = new AudioTrack(AudioManager.STREAM_MUSIC, samp_rate, AudioFormat
                        .CHANNEL_CONFIGURATION_MONO, AudioFormat.ENCODING_PCM_16BIT,
                        maxjitter, AudioTrack.MODE_STREAM);
                audioDecoder.play();
            }

            private void releaseAudioDecoder() {
                if (audioDecoder != null) {
                    audioDecoder.release();
                    audioDecoder = null;
                }
            }

            @Override
            public void run() {
                Log.w(TAG, "音频播放线程开始");
                try {
                    while (isRunning && !this.isInterrupted()) {
                        if (isPause) {
                            sleep(200);
                            continue;
                        }
                        EsStreamData esStreamData = (EsStreamData) audioBuffer
                                .removeObjectBlocking();
                        if (!isRunning || esStreamData == null) {
                            break;
                        }
                        if (audioBuffer.size() > 80) {
                            Log.e(TAG, "audio buffer to long");
                            audioBuffer.clear();
                            continue;
                        }
                        createAudioDecoder();
                        audioDecoder.write(esStreamData.getData(), 0, esStreamData.getData()
                                .length);

                    }
                } catch (InterruptedException e) {
                    Log.w(TAG, "音频播放线程中断");
                }
                releaseAudioDecoder();
                Log.w(TAG, "音频播放线程结束");
            }
        }

        private class OutputThread extends Thread {
            @Override
            public void run() {
                while (isRunning && !isInterrupted()) {
                    try {
                        if (!getConf) {
                            continue;
                        }

                        int outIndex = mediaCodecDecoder.dequeueOutputBuffer(info, 80000);
                        if (outData == null || outData.length < info.size) {
                            outData = new byte[info.size];
                        }
                        while (outIndex >= 0) {
                            ByteBuffer outputBuffer = null;

                            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP &&
                                    outputBuffers != null) {
                                outputBuffer = outputBuffers[outIndex];
                            } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES
                                    .LOLLIPOP) {
                                outputBuffer = mediaCodecDecoder.getOutputBuffer(outIndex);
                            }

                            if (outputBuffer != null) {
                                outputBuffer.position(info.offset);
                                outputBuffer.limit(info.offset + info.size);
                                outputBuffer.get(outData);

                                if (mColorFormatType == 0) {
                                    mColorFormatType = mediaCodecDecoder.getOutputFormat
                                            ().getInteger(MediaFormat.KEY_COLOR_FORMAT);

                                    Log.w(TAG, "colorFormatType = " + mColorFormatType);
                                }

                                int yL = w * h;
                                int uL = yL / 4;

                                if (mColorFormatType == MediaCodecInfo.CodecCapabilities
                                        .COLOR_FormatYUV420Planar) {  // 分片
                                    if (mOnYUVFrameDataCallback != null) {
                                        mOnYUVFrameDataCallback.onFrameData(w, h, outData, 0,
                                                yL, outData, yL, uL, outData, yL + uL, uL);
                                    }
                                } else
//                                    (mColorFormatType == MediaCodecInfo.CodecCapabilities
// .COLOR_FormatYUV420SemiPlanar)
                                {
                                    // 交叉
                                    if (mTmpVUBuffer == null || mTmpVUBuffer.length < uL * 2) {
                                        mTmpVUBuffer = new byte[uL * 2];
                                    }
                                    int offset = info.size * 2 / 3 - yL;
                                    if (offset < 0) {
                                        return;
                                    }
                                    YUVSP2YUVP(outData, yL + offset, uL * 2, mTmpVUBuffer);
                                    if (mOnYUVFrameDataCallback != null) {
                                        mOnYUVFrameDataCallback.onFrameData(w, h, outData, 0,
                                                yL, mTmpVUBuffer, 0, uL, mTmpVUBuffer, uL, uL);
                                    }

                                    // 不交叉
//                                    if (mOnYUVFrameDataCallback != null) {
//                                        mOnYUVFrameDataCallback.onFrameData(w, h, outData, 0,
//                                                yL, outData, yL, uL, outData, yL + uL, uL);
//                                    }
                                }

                                mediaCodecDecoder.releaseOutputBuffer(outIndex, false);

                                outIndex = mediaCodecDecoder.dequeueOutputBuffer(info, 80000);
                            }
                        }
                    } catch (Exception e) {
                        Log.w(TAG, StringUtil.getStackTraceAsString(e));
                    }
                }
            }
        }
    }

    // 录像线程
    private class RecordThread extends Thread {
        // 录像数据
        private BlockingBuffer recordBuffer = new BlockingBuffer(BUFFER_MAX_SIZE,
                BUFFER_WARNING_SIZE);
        //        private FlvSave flvSave;
        private Mp4Save mp4Save;
        private String fileName;
        private byte[] sps;  // 需要写入的sps

        private long aacEncoder;

        private byte[] audioDecodeBuffer;

        private RecordThread() {

        }

        public RecordThread(String fileName) {
            this.fileName = fileName;
        }

        private void initAACEncoder() {
            if (aacEncoder == 0) {
                aacEncoder = AACEncoderInit(8000);
            }
        }

        private void uninitAACEncoder() {
            if (aacEncoder != 0) {
                AACEncoderUninit(aacEncoder);
                aacEncoder = 0;
            }
        }

        public boolean addBuffer(Object object) {
            return recordBuffer.addObject(object);
        }

        public void shutDown() {
            this.interrupt();
            try {
                this.join();
            } catch (InterruptedException e) {
                Log.e(TAG, StringUtil.getStackTraceAsString(e));
            }
        }

        @Override
        public void run() {
            if (this.fileName == null) {
                return;
            }
            Log.i(TAG, "录像线程开始...");

            boolean findSps = false;  // 是否找到sps
            boolean confWrited = false;  // 配置是否写入

            if (mp4Save == null) {
                try {
                    mp4Save = new Mp4Save(this.fileName);
                } catch (FileNotFoundException e) {
                    e.printStackTrace();
                    return;
                }
                if (!mp4Save.writeStart()) {
                    return;
                }
            }
            try {
                while (isRunning && !isInterrupted()) {
                    EsStreamData esStreamData = (EsStreamData) recordBuffer.removeObjectBlocking();
                    if (!isRunning || esStreamData == null) {
                        break;
                    }

                    int dataType = esStreamData.getDataType();
                    byte[] data = esStreamData.getData();

                    if (!findSps && dataType == DATA_TYPE_VIDEO_SPS) {  // 找到sps
                        sps = new byte[data.length - 4];
                        System.arraycopy(data, 4, sps, 0, sps.length);
                        findSps = true;
                    } else if (findSps && !confWrited && dataType == DATA_TYPE_VIDEO_PPS) {
//                        flvSave.writeConfiguretion(sps, 0, sps.length, mData, 4,
//                                mData.length - 4, esStreamData.getTimestamp());
                        mp4Save.writeConfiguretion(getWidth(), getHeight(), getFramerate(), sps, 0,
                                sps.length, data, 4, data.length - 4);
                        confWrited = true;
                    } else if (confWrited && (dataType == DATA_TYPE_VIDEO_IFRAME || dataType ==
                            DATA_TYPE_VIDEO_PFRAME) || dataType == DATA_TYPE_VIDEO_OTHER) {
//                        flvSave.writeNalu(dataType == DATA_TYPE_VIDEO_IFRAME, mData, 4, mData
//                                .length - 4, esStreamData.getTimestamp());
                        mp4Save.writeNalu(dataType == DATA_TYPE_VIDEO_IFRAME, data, 4, data
                                .length - 4);
                    } else if (dataType == DATA_TYPE_AUDIO) {
                        initAACEncoder();
                        if (aacEncoder != 0) {
                            if (audioDecodeBuffer == null) {
                                audioDecodeBuffer = new byte[1024 * 2];
                            }
                            int decodeLen = AACEncodePCM2AAC(aacEncoder, data, data.length,
                                    audioDecodeBuffer);
//                            Log.e(TAG, "decodeLen=" + decodeLen);
                            if (confWrited && decodeLen > 0) {  // 转aac成功
                                mp4Save.writeAAC(audioDecodeBuffer, 0, decodeLen);
                            }
                        }
                    }
                }
            } catch (Exception e) {
                Log.w(TAG, StringUtil.getStackTraceAsString(e));
            }
            uninitAACEncoder();
//            if (flvSave != null) {
//                flvSave.save();
//            }
            if (mp4Save != null) {
                mp4Save.save();
            }

            Log.i(TAG, "录像线程结束...");
        }
    }

    // es流元数据
    class EsStreamData implements Comparable<EsStreamData> {
        int dataType;  // 数据类型
        int payloadType;
        int timestamp;
        long pts;
        byte[] data;

        private EsStreamData() {

        }

        public EsStreamData(int dataType, int payloadType, int timestamp, long pts, byte[] data,
                            int begin,
                            int len) {
            this.dataType = dataType;
            this.payloadType = payloadType;
            this.timestamp = timestamp;
            this.pts = pts;
            if (data != null) {
                this.data = new byte[len];
                System.arraycopy(data, begin, this.data, 0, this.data.length);
            }
        }

        public long getPts() {
            return pts;
        }

        public int getDataType() {
            return dataType;
        }

        public int getPayloadType() {
            return payloadType;
        }

        public int getTimestamp() {
            return timestamp;
        }

        public byte[] getData() {
            return data;
        }

        @Override
        public int compareTo(@NonNull EsStreamData another) {
            if (this == another) {
                return 0;
            }
            // 由于修改时间的精度不够，在同一秒内的文件无法比较出其大小，需要用文件名来比较
            if (pts > another.pts) {
                return -1;
            } else if (pts == another.pts) {
                return 0;
            } else {
                return 1;
            }
        }
    }
}

