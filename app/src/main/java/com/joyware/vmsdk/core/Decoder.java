package com.joyware.vmsdk.core;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.os.AsyncTask;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;

import com.joyware.vmsdk.ScreenshotCallback;
import com.joyware.vmsdk.VmType;
import com.joyware.vmsdk.util.G711;
import com.joyware.vmsdk.util.Mp4Save;
import com.joyware.vmsdk.util.OpenGLESUtil;
import com.joyware.vmsdk.util.PsStreamUtil;
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
    // 应用启动时，加载VmNet动态库
    static {
        System.loadLibrary("JWEncdec");  // 某些设备型号必须要加上依赖的动态库
        System.loadLibrary("VmPlayer");
    }

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
    private SurfaceHolder surfaceHolder;
    private Context context;  // 上下文，获取是否支持OpenGLES2.0
    private boolean openAudio = false;

    private volatile boolean isRunning = false;

    private HandleFrameThread handleFrameThread;
    private PlayThread playThread;
    private RecordThread recordThread;

    private boolean closeOpengles = false;
    private boolean closeSmooth = false;

    private Decoder() {

    }

    public Decoder(int decodeType) {
        this.decodeType = decodeType;
    }

    public Decoder(int decodeType, boolean closeOpengles, boolean closeSmooth, boolean autoPlay,
                   boolean openAudio,
                   SurfaceHolder surfaceHolder, Context context) {
        Log.i(TAG, "构造解码器 decodeType=" + decodeType + ", autoPlay=" + autoPlay);
        this.decodeType = decodeType;
        this.closeOpengles = closeOpengles;
        this.closeSmooth = closeSmooth;
        this.surfaceHolder = surfaceHolder;
        this.context = context;
        this.openAudio = openAudio;

        if (autoPlay) {
            startPlay();
        }
    }

    public boolean setSurfaceHolder(SurfaceHolder surfaceHolder) {
        if (surfaceHolder != null && surfaceHolder.getSurface() != null && surfaceHolder
                .getSurface().isValid()) {
            if (playThread != null && playThread.displayThread != null) {
                this.surfaceHolder.removeCallback(playThread.displayThread);
            }
            this.surfaceHolder = surfaceHolder;
            if (playThread != null && playThread.displayThread != null) {
                this.surfaceHolder.addCallback(playThread.displayThread);
            }
            return true;
        }
        return false;
    }

    private static native long DecoderInit(int payloadType);

    private static native void DecoderUninit(long decoderHandle);

    private static native int DecodeNalu2RGB(long decoderHandle, byte[] inData, int inLen, byte[]
            outData, FrameConfHolder frameConfHolder);

    private static native int DecodeNalu2YUV(long decoderHandle, byte[] inData, int inLen, byte[]
            yData, byte[] uData, byte[] vData, FrameConfHolder frameConfHolder);

    private static native int GetFrameWidth(long decoderHandle);

    private static native int GetFrameHeight(long decoderHandle);

    private static native long AACEncoderInit(int samplerate);

    private static native void AACEncoderUninit(long encoderHandle);

    private static native int AACEncodePCM2AAC(long encoderHandle, byte[] inData, int inLen, byte[]
            outData);

    private static native long RenderInit(Surface surface);

    private static native void RenderUninit(long renderHandle);

    private static native void RenderSurfaceCreated(long renderHandle);

    private static native void RenderSurfaceDestroyed(long renderHandle);

    private static native void RenderSurfaceChanged(long renderHandle, int width, int height);

    private static native void RenderScaleTo(long renderHandle, boolean enable, int centerX, int
            centerY,
                                             float widthScale, float heightScale);

    private static native int DrawYUV(long renderHandle, byte[] yData, int yStart, int yLen,
                                      byte[] uData, int uStart, int uLen, byte[] vData, int
                                              vStart, int vLen, int width, int height);

    private static native int OfflineScreenRendering(long renderHandle, byte[] yData, int yLen,
                                                     byte[] uData, int uLen, byte[] vData, int
                                                             vLen, int width, int height, byte[]
                                                             outData);

    public boolean addBuffer(Object object) {
        if (handleFrameThread != null) {
            return handleFrameThread.addBuffer(object);
        }
        return false;
    }

    public int getWidth() {
        if (playThread != null) {
            return playThread.w;
        }
        return 0;
    }

    public int getHeight() {
        if (playThread != null) {
            return playThread.h;
        }
        return 0;
    }

    public void scaleTo(boolean enable, int centerX, int centerY, float widthScale, float
            heightScale) {
        if (playThread != null && playThread.displayThread != null) {
            RenderScaleTo(playThread.displayThread.renderHandle, enable, centerX, centerY,
                    widthScale, heightScale);
        }
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

    public synchronized boolean startPlay() {
        if (handleFrameThread != null && playThread != null) {
            return true;
        }
        isRunning = true;
        try {
            if (handleFrameThread == null) {
                handleFrameThread = new HandleFrameThread();
                handleFrameThread.start();
            }
            if (playThread == null) {
                playThread = new PlayThread(decodeType, closeOpengles, this.closeSmooth);
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

        if (handleFrameThread != null && recordThread == null) {
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

        PsStreamUtil psStreamUtil = new PsStreamUtil();

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
            boolean isFirst = true;
            boolean reset = true;

            try {
                while (isRunning && !isInterrupted()) {
                    StreamData streamData = (StreamData) streamBuffer.removeObjectBlocking();
                    if (!isRunning || streamData == null) {
                        break;
                    }
                    if ((playThread == null && recordThread == null)) {
                        continue;
                    }

                    int streamType = streamData.getStreamType();

                    // 需要发送的数据
                    int dataType = DATA_TYPE_VIDEO_PFRAME;
                    byte[] data;
                    int begin = 0;
                    int len;
                    int payloadType = streamData.getPayloadType();
//                    Log.e(TAG, "payloadType=" + payloadType);
                    int timestamp = streamData.getTimeStamp();

                    if (reset) {
                        resetVideoBuffer(payloadType, timestamp);
                        reset = false;
                    }

                    if (streamType == VmType.STREAM_TYPE_AUDIO) {  // 如果是音频，则不需要拼包和分包
                        dataType = DATA_TYPE_AUDIO;
                        data = streamData.getBuffer();
                        begin = 0;
                        len = streamData.getBuffer().length;

                        sendData(dataType, payloadType, timestamp, data, begin, len);
                    } else if (streamType == VmType.STREAM_TYPE_VIDEO) {  // 视频数据
                        // 这里需要判断是否是PS流，如果是复合的PS流，还需要再分出视频和音频流
                        boolean isVideo = true;
                        if (payloadTypeOld == PAYLOAD_TYPE_PS) {
                            begin = psStreamUtil.filterPsHeader(streamData.getBuffer(), 0,
                                    streamData.getBuffer().length);
//                            Log.e(TAG, "filterPsHeader begin=" + begin);
                            if (begin < streamData.getBuffer().length) {  // 表示有数据可以使用
//                                Log.e(TAG, "有效长度=" + (streamData.getBuffer().length - begin));
                                if (!psStreamUtil.isFindDataStart()) {
                                    continue;
                                }

                                if (psStreamUtil.isVideo()) {
                                    isVideo = true;
//                                    Log.e(TAG, "视频数据 beginNew=" + begin);
                                } else {  // 是音频
                                    isVideo = false;

                                    if (!isFirst) {  // 发现第一帧之后再发送音频
                                        int lenNow = streamData.getBuffer().length - begin;
                                        byte[] dataForDecode = new byte[lenNow * 2];
                                        int audioLen = G711.decode(streamData.getBuffer(), begin,
                                                lenNow, dataForDecode);

                                        dataType = DATA_TYPE_AUDIO;
                                        sendData(dataType, payloadType, timestamp, dataForDecode, 0,
                                                audioLen);
                                    }
//                                    Log.e(TAG, "音频数据 beginNew=" + begin);
                                }
                            } else {
//                                Log.e(TAG, "无可用数据");
                            }
                        }

                        if (isVideo) {
                            int packetLen = streamData.getBuffer().length;
                            streamBufUsed = begin;

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
                                    if (isFirst && (videoBuf[4] & 0x1F) != 0x07) {  //
                                        // 一开始如果不是I帧，就全部丢弃，加快显示速度，还能不用看到绿屏
                                        // 发送完之后再重置视频缓存
                                        resetVideoBuffer(payloadType, timestamp);
                                    } else {
                                        isFirst = false;
                                        boolean send = true;
//                                        Log.e(TAG, "videoBuf[4]=" + videoBuf[4]);
                                        if ((videoBuf[4] & 0x1F) == 0x07) {  // sps
                                            dataType = DATA_TYPE_VIDEO_SPS;
                                        } else if ((videoBuf[4] & 0x1F) == 0x08) {  // pps
                                            dataType = DATA_TYPE_VIDEO_PPS;
                                        } else if ((videoBuf[4] & 0x1F) == 0x05) {  // I帧
                                            dataType = DATA_TYPE_VIDEO_IFRAME;
                                        } else if ((videoBuf[4] & 0x1F) == 0x06) {
                                            dataType = DATA_TYPE_VIDEO_OTHER;
                                        } else {  // 否则都当做P帧
                                            dataType = DATA_TYPE_VIDEO_PFRAME;
                                        }
//                                        Log.e(TAG, "videoBuf[4]=" + videoBuf[4]);
                                        data = videoBuf;
                                        begin = 0;
                                        len = videoBufUsed - 4;  // 最后会多4个字节的0x00 00 00 01

//                                        Log.e(TAG, "一帧数据");
                                        if (send) {
                                            sendData(dataType, payloadType, timestamp, data,
                                                    begin, len);
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

        private void sendData(int dataType, int payloadType, int timestamp, byte[] data, int begin,
                              int len) {
            // 需要发送的数据不为空时，需要发送
            if (data != null) {
                // 解析出es元数据后，看线程是否需要使用该数据，发送数据到相应的线程
                if (playThread != null) {
                    EsStreamData esStreamData = new EsStreamData(dataType, payloadTypeOld,
                            timestampOld, data, begin, len);
                    playThread.addBuffer(esStreamData);
                }
                if (recordThread != null) {
                    EsStreamData esStreamData = new EsStreamData(dataType, payloadTypeOld,
                            timestampOld, data, begin, len);
                    recordThread.addBuffer(esStreamData);
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

        // 根据0x00 00 00 01 来进行分段
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
                if (SockBuf[SockBufUsed + 0] == 0x00 && SockBuf[SockBufUsed + 1] == 0x00 &&
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
        private boolean closeOpengles = false;
        private boolean closeSmooth = false;
        //        private SurfaceHolder surfaceHolder;
        private boolean isTv = false;

        // 硬件解码相关
        private MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
        private int w = 1080;
        private int h = 720;
        // 帧率
        private int framerate = 25;
        // 是否获取到配置
        private boolean getConf = false;

        // 帧间隔 默认40毫秒
        private long interval = 40;
        private String mime = "video/avc";
        private MediaFormat mediaFormat = MediaFormat.createVideoFormat(mime, w, h);
        private MediaCodec mediaCodecDecoder;  //硬解码器
        private ByteBuffer[] inputBuffers;

        private int currentPayloadType;

        // private boolean openAudio = false;

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

        private Display displayThread;

        private PlayAudio playAudioThread;

        private long timestamp;

        // 播放数据
        private BlockingBuffer playBuffer = new BlockingBuffer(BUFFER_MAX_SIZE,
                BUFFER_WARNING_SIZE);

        // surface是否被创建
        private boolean surfaceCreated = false;
        private int surfaceWidth;
        private int surfaceHeight;
        private boolean isPause = false;

        public PlayThread(int decodeType, boolean closeOpengles, boolean closeSmooth) {
            this.decodeType = decodeType;
            this.closeOpengles = closeOpengles;
            this.closeSmooth = closeSmooth;
//            this.surfaceHolder = surfaceHolder;
            if (surfaceHolder != null) {
                this.surfaceCreated = surfaceHolder.getSurface().isValid();
                this.surfaceWidth = surfaceHolder.getSurfaceFrame().width();
                this.surfaceHeight = surfaceHolder.getSurfaceFrame().height();
            }
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
                // 窗口不存在则直接跳过
                if (!surfaceHolder.getSurface().isValid()) {
                    return;
                }

                if (decodeType == VmType.DECODE_TYPE_SOFTWARE) {
                    if (decoderHandle == 0) {
                        // 判断是否支持open gl es 2.0
                        if (context != null) {
                            Log.i(TAG, "支持OpenGLES2.0");
                            if (OpenGLESUtil.detectOpenGLES20(context)) {
                                // 如果没有强制关闭opengles，那么就使用yuv渲染模式
                                if (!closeOpengles) {
                                    Log.i(TAG, "开启yuv渲染模式");
                                } else {
                                    useOpengles = false;
                                    Log.i(TAG, "yuv渲染模式已被强制关闭！");
                                }
                            } else {
                                Log.i(TAG, "不支持OpenGLES2.0");
                            }
                        }

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

                        decoderHandle = DecoderInit(payloadType);
                        Log.i(TAG, "decoderHandle=" + decoderHandle);
                    }
                } else {
                    if (mediaCodecDecoder == null && isRunning) {
                        try {
                            if (surfaceHolder != null && surfaceHolder.getSurface() != null) {
//              Log.e(TAG, "createDecoder");
                                mediaCodecDecoder = MediaCodec.createDecoderByType(mime);
                                mediaFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT,
                                        MediaCodecInfo.CodecCapabilities
                                                .COLOR_FormatYUV420PackedSemiPlanar);
//                                mediaCodecDecoder.configure(mediaFormat, surfaceHolder.getSurface(),
//                                        null, 0);
                                mediaCodecDecoder.configure(mediaFormat, null, null, 0);  //
// todo:不传入surface，只用来解码，自己来显示
                                mediaCodecDecoder.start();

                                inputBuffers = mediaCodecDecoder.getInputBuffers();
                            }
                        } catch (Exception e) {
                            if (decodeType == VmType.DECODE_TYPE_INTELL) {
                                decodeType = VmType.DECODE_TYPE_SOFTWARE;
                            }
                            Log.e(TAG, StringUtil.getStackTraceAsString(e));
                        }
                    }
                }
                if (displayThread == null) {
                    displayThread = new Display(this.closeSmooth);
                    displayThread.start();
                }
            }
        }

        private void releaseDecoder() {
            if (decoderHandle != 0) {
                Log.e(TAG, "DecoderUninit " + decoderHandle);
                DecoderUninit(decoderHandle);
                decoderHandle = 0;
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
            if (displayThread != null) {
                displayThread.shutDown();
                displayThread = null;
            }
            releaseAudioThread();
        }

        //    FileOutputStream fileOutputStream;
        @Override
        public void run() {
            Log.w(TAG, "解码线程开始...");

            try {
                while (isRunning && !this.isInterrupted()) {
                    EsStreamData esStreamData = (EsStreamData) playBuffer.removeObjectBlocking();
                    if (!isRunning || esStreamData == null) {
                        break;
                    }

                    // 窗口不存在则直接跳过
                    if (!this.surfaceCreated) {
                        releaseDecoder();
                        continue;
                    }

                    int dataType = esStreamData.getDataType();

                    // 如果队列中大于100帧没有处理，则做丢帧处理，只对p帧丢包，别的包不可以丢
                    if (playBuffer.size() > 100 && (dataType == DATA_TYPE_VIDEO_PFRAME ||
                            dataType == DATA_TYPE_AUDIO)) {
                        // todo: 发现问题，如果夹带着有音频包并且需要播放音频，这一个线程吃不消，
                        // 一旦队列超过100后，丢失掉视频帧也无法将队列数量降下来，将这个放音频处理上面看看
                        Log.e(TAG, "buffer over 100, miss data");
                        continue;
                    }

                    // 如果是音频的话，则只需要执行这段代码即可
                    if (dataType == DATA_TYPE_AUDIO) {
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

                    // 如果是没显示过，那么需要从sps开始解码
                    if (!showed && dataType != DATA_TYPE_VIDEO_SPS) {
                        continue;
                    }

                    showed = true;

                    createDecoder(esStreamData.getPayloadType());

                    if ((decodeType == VmType.DECODE_TYPE_INTELL || decodeType == VmType
                            .DECODE_TYPE_HARDWARE) && mediaCodecDecoder == null) {
                        continue;
                    }

                    if ((decodeType == VmType.DECODE_TYPE_SOFTWARE) && decoderHandle == 0) {
                        continue;
                    }

                    byte[] data = esStreamData.getData();

//                    Log.e(TAG, "type=" + (data[4] & 0x1f));
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
//            lastIFrameBuffer = new byte[data.length];
//            System.arraycopy(data, 0, lastIFrameBuffer, 0, data.length);
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
                        if (useOpengles) {
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
                                if (canShow > 0 && displayThread != null) {
                                    // 异步处理
                                    YUVFrameData yuvFrameData = new YUVFrameData(yBuffer,
                                            uBuffer, vBuffer, w, h);
//                                    Log.e(TAG, "可以显示");
                                    displayThread.addBuffer(yuvFrameData);
                                }
                            }
                        } else {
                            canShow = DecodeNalu2RGB(decoderHandle, data, data.length, pixelBuffer,
                                    frameConfHolder);
//              Log.e(TAG, "canShow=" + canShow + " , dataType=" + dataType);
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
                                    int size = w * h * 2;
                                    // pixelBuffer接收解码数据的缓存
                                    pixelBuffer = new byte[size];
//                  for (int i = 0; i < size; ++i) {
//                    pixelBuffer[i] = (byte) 0x00;
//                  }

                                    // 重新解一次码
                                    canShow = DecodeNalu2RGB(decoderHandle, data, data.length,
                                            pixelBuffer,
                                            frameConfHolder);
                                }
                                // 可以显示
                                if (canShow > 0) {
                                    // 异步处理
                                    // draw(surfaceHolder);
                                    if (displayThread != null) {
                                        RGBFrameData rgbFrameData = new RGBFrameData(pixelBuffer,
                                                w, h);
                                        displayThread.addBuffer(rgbFrameData);
                                    }
                                }
                            }
                        }
                    } else {  // 智能解码或者硬件解码
                        int inputBufferIndex;

                        // 窗口不存在则直接跳过
                        if (!this.surfaceCreated) {
                            releaseDecoder();
                            continue;
                        }

                        try {
                            if (isTv) {
                                inputBufferIndex = mediaCodecDecoder.dequeueInputBuffer(1000);
                            } else {
                                inputBufferIndex = mediaCodecDecoder.dequeueInputBuffer(80000);
                            }

                            if (inputBufferIndex >= 0) {
                                ByteBuffer inputBuffer = inputBuffers[inputBufferIndex];
                                inputBuffer.clear();
                                inputBuffer.put(data, 0, data.length);

                                int flags;
                                switch (dataType) {
                                    case DATA_TYPE_VIDEO_SPS:
                                    case DATA_TYPE_VIDEO_PPS:
                                        flags = MediaCodec.BUFFER_FLAG_CODEC_CONFIG;
                                        break;
                                    default:
                                        flags = MediaCodec.BUFFER_FLAG_SYNC_FRAME;
                                        timestamp = (timestamp + 40000) % Long.MAX_VALUE;
                                        break;
                                }

                                // 窗口不存在则直接跳过
                                if (!this.surfaceCreated) {
                                    releaseDecoder();
                                    continue;
                                }
                                mediaCodecDecoder.queueInputBuffer(inputBufferIndex, 0,
                                        data.length, timestamp, flags);

                                if (displayThread != null) {
                                    displayThread.addInputCount();
                                }
                            }
                        } catch (Exception e) {
                            Log.w(TAG, StringUtil.getStackTraceAsString(e));
                            releaseDecoder();
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

        private class Display extends Thread implements SurfaceHolder.Callback {
            private long renderHandle;
            private int width;
            private int height;
            private Bitmap videoBitmap;
            private ByteBuffer rgbBuffer;
            private long lastDisplay;

            // 解码出来后的数据队列
            private BlockingBuffer frameBuffer = new BlockingBuffer(15, 10);  // 最多放延迟25帧，再多的话，内存会崩溃

            // 硬件解码时，framebuffer永远为空，那么在视频间隔平滑时，会有问题，所以加上这个
            private int inputCount;
            private boolean closeSmooth = false;

            public Display(boolean closeSmooth) {
                this.closeSmooth = closeSmooth;
                if (surfaceHolder != null) {
                    surfaceHolder.addCallback(this);
                }
            }

            public boolean addBuffer(Object object) {
                return frameBuffer.addObjectForce(object);
            }

            public synchronized void addInputCount() {
                ++inputCount;
            }

            private synchronized int getInputCount() {
                return inputCount;
            }

            private synchronized void decInputCount() {
                if (inputCount > 0) {
                    --inputCount;
                }
            }

            public void shutDown() {
                this.interrupt();
                try {
                    this.join();
                } catch (InterruptedException e) {
                    Log.e(TAG, StringUtil.getStackTraceAsString(e));
                }
                if (surfaceHolder != null) {
                    surfaceHolder.removeCallback(this);
                }
            }

            private void drawRGB(SurfaceHolder surfaceHolder, byte[] frameData, int width, int
                    height) {
                if (this.width != width || this.height != height) {
                    this.width = width;
                    this.height = height;
                    videoBitmap = Bitmap.createBitmap(this.width, this.height, Bitmap.Config
                            .RGB_565);
                }
                rgbBuffer = ByteBuffer.wrap(frameData, 0, frameData.length);
                videoBitmap.copyPixelsFromBuffer(rgbBuffer);

                // 窗口不存在则直接跳过
                if (!surfaceCreated) {
                    return;
                }

                try {
                    Canvas canvas = surfaceHolder.lockCanvas(null);

                    if (canvas != null) {
                        // 适应surfaceview屏幕大小
                        int displayWidth = surfaceWidth > 0 ? surfaceWidth : surfaceHolder
                                .getSurfaceFrame()
                                .width();
                        int displayHeight = surfaceHeight > 0 ? surfaceHeight : surfaceHolder
                                .getSurfaceFrame
                                        ().height();
                        float scaleWidth = ((float) displayWidth) / w;
                        float scaleHeight = ((float) displayHeight) / h;
                        Matrix matrix = new Matrix();
                        matrix.postScale(scaleWidth, scaleHeight);
                        // 显示到画布
                        canvas.drawBitmap(videoBitmap, matrix, null);
                    }

                    surfaceHolder.unlockCanvasAndPost(canvas);
                } catch (Exception e) {
                }
            }

            private void createRender() {
                if (useOpengles && renderHandle == 0 && surfaceCreated) {
                    renderHandle = RenderInit(surfaceHolder.getSurface());
                }
                // 同步大小
                if (renderHandle != 0) {
                    RenderSurfaceChanged(renderHandle, surfaceWidth, surfaceHeight);
                }
            }

            private void releaseRender() {
                if (renderHandle != 0) {
                    RenderUninit(renderHandle);
                    renderHandle = 0;
                }
            }

            @Override
            public void run() {
                Log.w(TAG, "显示线程开始...");
                // 显示线程是随着窗口生成或释放的
                try {
                    while (isRunning && !this.isInterrupted()) {
                        if (isPause) {
                            sleep(200);
                            continue;
                        }
                        boolean display = false;
                        if (decodeType == VmType.DECODE_TYPE_SOFTWARE) {  // 使用软解码显示
//              try {
                            FrameData frameData = (FrameData) frameBuffer.removeObjectBlocking();
                            // 窗口不存在则直接跳过
                            if (!surfaceCreated) {
                                lastDisplay = System.nanoTime();
                                releaseRender();
                                continue;
                            }
                            int frameType = frameData.getFrameType();

                            if (frameType == FrameData.FRAME_TYPE_RGB) {
//                // 窗口不存在则直接跳过
//                if (!surfaceCreated) {
//                  lastDisplay = System.nanoTime();
//                  continue;
//                }

                                RGBFrameData rgbFrameData = (RGBFrameData) frameData;
                                drawRGB(surfaceHolder, rgbFrameData.getData(), rgbFrameData
                                        .getWidth(), rgbFrameData
                                        .getHeight());
                            } else if (frameType == FrameData.FRAME_TYPE_YUV) {
//                // 窗口不存在则直接跳过
//                if (!surfaceCreated) {
//                  lastDisplay = System.nanoTime();
//                  continue;
//                }

                                createRender();

                                YUVFrameData yuvFrameData = (YUVFrameData) frameData;
                                byte[] yData = yuvFrameData.getyData();
                                byte[] uData = yuvFrameData.getuData();
                                byte[] vData = yuvFrameData.getvData();

                                DrawYUV(renderHandle, yData, 0, yData.length, uData, 0, uData.length,
                                        vData, 0, vData.length, yuvFrameData.getWidth(), yuvFrameData
                                                .getHeight());
//                DrawYUV(renderHandle, yData, yData.length, uData, uData.length, vData, vData
//                    .length, surfaceWidth, surfaceHeight);
                            }
                            display = true;
//
//              } catch (Exception e) {
//                Log.e(TAG, StringUtil.getStackTraceAsString(e));
//                break;
//              }
                        } else {  // 硬件解码显示
                            if (mediaCodecDecoder != null && surfaceCreated) {
                                try {
                                    ByteBuffer[] outputBuffers = mediaCodecDecoder
                                            .getOutputBuffers();

                                    int outIndex = mediaCodecDecoder.dequeueOutputBuffer(info, 0);

                                    byte[] outData = new byte[info.size];
                                    if (outIndex >= 0) {
                                        ByteBuffer outputBuffer = outputBuffers[outIndex];

                                        outputBuffer.position(info.offset);
                                        outputBuffer.limit(info.offset + info.size);
                                        outputBuffer.get(outData);

                                        createRender();

                                        int yL = w * h;
                                        int uL = yL / 4;
                                        DrawYUV(renderHandle, outData, 0, yL, outData, yL,
                                                uL, outData, yL + uL, uL, w, h);

                                        mediaCodecDecoder.releaseOutputBuffer(outIndex, true);
                                        decInputCount();
                                        display = true;
                                    }
                                } catch (Exception e) {
                                    Log.w(TAG, StringUtil.getStackTraceAsString(e));
                                    // releaseDecoder();
                                }
                            }
                        }

                        // 如果关闭平滑
                        if (this.closeSmooth) {
                            continue;
                        }

                        // 流畅显示的解决方案
                        // 当缓冲区帧过多时，不调用该流畅方案，即解码时尽量保证匀速
                        while (isRunning && !this.isInterrupted() && display && (frameBuffer.size
                                () < 5) && getInputCount() < 5 && lastDisplay > 0) {
                            // －10毫秒的是为了应对，网络波动，这样在网络先堵塞，后流畅的情况下，会在短时间内接收到大量包，如果还平滑的话会延时太大
                            if (System.nanoTime() - lastDisplay > (interval * 1000000 - 5000000)) {
                                lastDisplay = System.nanoTime();
                                break;
                            }
                            // 还是加上个挂起，增加一下cpu的使用率，这个时间不能太大，不然在arm环境下，线程竞争回cpu使用权会很慢
                            sleep(1);
                        }

                        if (lastDisplay == 0) {
                            lastDisplay = System.nanoTime();
                        }
                    }
                } catch (InterruptedException e) {
                    Log.w(TAG, "显示线程中断!");
                    Log.w(TAG, StringUtil.getStackTraceAsString(e));
                }
                releaseRender();
                Log.w(TAG, "显示线程结束...");
            }

            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                Log.w(TAG, "surfaceCreated");
                surfaceCreated = true;
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                Log.w(TAG, "surfaceChanged");
                surfaceWidth = width;
                surfaceHeight = height;
            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {
                Log.w(TAG, "surfaceDestroyed");
                surfaceCreated = false;
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

//            if (flvSave == null) {
//                try {
//                    flvSave = new FlvSave(fileName);
//                } catch (FileNotFoundException e) {
//                    e.printStackTrace();
//                    return;
//                }
//                if (!flvSave.writeStart(true)) {  // 如果写头失败的话，返回
//                    return;
//                }
//            }
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
//                        flvSave.writeConfiguretion(sps, 0, sps.length, data, 4,
//                                data.length - 4, esStreamData.getTimestamp());
                        mp4Save.writeConfiguretion(getWidth(), getHeight(), getFramerate(), sps, 0,
                                sps.length, data, 4, data.length - 4);
                        confWrited = true;
                    } else if (confWrited && (dataType == DATA_TYPE_VIDEO_IFRAME || dataType ==
                            DATA_TYPE_VIDEO_PFRAME) || dataType == DATA_TYPE_VIDEO_OTHER) {
//                        flvSave.writeNalu(dataType == DATA_TYPE_VIDEO_IFRAME, data, 4, data
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
    class EsStreamData {
        int dataType;  // 数据类型
        int payloadType;
        int timestamp;
        byte[] data;

        private EsStreamData() {

        }

        public EsStreamData(int dataType, int payloadType, int timestamp, byte[] data, int begin,
                            int len) {
            this.dataType = dataType;
            this.payloadType = payloadType;
            this.timestamp = timestamp;
            if (data != null) {
                this.data = new byte[len];
                System.arraycopy(data, begin, this.data, 0, this.data.length);
            }
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
    }
}

