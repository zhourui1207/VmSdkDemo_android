package com.jxll.vmsdk.core;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.media.MediaCodec;
import android.media.MediaFormat;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;

import com.jxll.vmsdk.VmType;
import com.jxll.vmsdk.util.FlvSave;
import com.jxll.vmsdk.util.OpenGLESUtil;
import com.jxll.vmsdk.util.StringUtil;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

/**
 * Created by zhourui on 16/10/28.
 * 解码器，包含软硬解视频，截图，实时录像，打开关闭音频等功能
 */

public class Decoder {
  private final String TAG = "Decoder";

  // 应用启动时，加载VmNet动态库
  static {
    System.loadLibrary("VmPlayer");
  }

  private final int BUFFER_MAX_SIZE = 500;
  private final int BUFFER_WARNING_SIZE = 400;

  // es数据类型
  private final int DATA_TYPE_UNKNOW = 0;  // 未知类型

  private final int DATA_TYPE_VIDEO_SPS = 1;   // sps数据
  private final int DATA_TYPE_VIDEO_PPS = 2;  // pps数据
  private final int DATA_TYPE_VIDEO_IFRAME = 3;  // I帧数据
  private final int DATA_TYPE_VIDEO_PFRAME = 4;  // P帧数据

  private final int DATA_TYPE_AUDIO = 11;  // 音频数据

  private int decodeType = VmType.DECODE_TYPE_INTELL;
  private SurfaceHolder surfaceHolder;
  private Context context;  // 上下文，获取是否支持OpenGLES2.0

  private boolean isRunning = false;

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
                 SurfaceHolder
      surfaceHolder, Context context) {
    Log.i(TAG, "构造解码器 decodeType=" + decodeType + ", autoPlay=" + autoPlay);
    this.decodeType = decodeType;
    this.closeOpengles = closeOpengles;
    this.closeSmooth = closeSmooth;
    this.surfaceHolder = surfaceHolder;
    this.context = context;
    if (autoPlay) {
      startPlay();
    }
  }

  public boolean addBuffer(Object object) {
    if (handleFrameThread != null) {
      return handleFrameThread.addBuffer(object);
    }
    return false;
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
        playThread = new PlayThread(decodeType, closeOpengles, this.closeSmooth, surfaceHolder);
        playThread.start();
      }
    } catch (Exception e) {
      return false;
    }
    return true;
  }

  public synchronized void stopPlay() {
    if (handleFrameThread == null && playThread == null) {
      return;
    }
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

  public synchronized boolean startRecord(String fileName) {
    if (recordThread != null && handleFrameThread != null) {
      return true;
    }

    FileOutputStream fileOutputStream;
    try {
      fileOutputStream = new FileOutputStream(fileName);
    } catch (FileNotFoundException e) {
      Log.e(TAG, StringUtil.getStackTraceAsString(e));
      return false;
    }

    if (fileOutputStream == null) {
      return false;
    }

    isRunning = true;
    try {
      if (handleFrameThread == null) {
        handleFrameThread = new HandleFrameThread();
        handleFrameThread.start();
      }
      if (recordThread == null) {
        recordThread = new RecordThread(fileOutputStream);
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

  public synchronized boolean screenshot(String fileName) {
    if (recordThread != null && handleFrameThread != null) {
      return true;
    }

    if (playThread == null) {
      return false;
    }

    return playThread.screenshot(fileName);
  }

  public void pauseDisplay() {
    if (playThread != null) {
      playThread.pauseDisplay();
    }
  }

  public void continueDisplay() {
    if (playThread != null) {
      playThread.continueDisplay();
    }
  }

  @Override
  protected void finalize() throws Throwable {
    super.finalize();
    stopRecord();
    stopPlay();
  }

  // 处理帧数据线程
  private class HandleFrameThread extends Thread {
    private BlockingBuffer streamBuffer = new BlockingBuffer(BUFFER_MAX_SIZE,
        BUFFER_WARNING_SIZE);  // 码流数据

    public boolean addBuffer(Object object) {
      return streamBuffer.addObjectForce(object);
    }

    // 既有音频又有视频是为了兼容混合流
    byte[] videoBuf = new byte[409800]; // 400k  es元数据缓存，psp，pps，i帧，p帧，音频等
    int videoBufUsed;

    int streamBufUsed = 0;

    // 最新的4个字节内容
    int trans = 0xFFFFFFFF;

    // 保存包信息
    int payloadTypeOld;
    int timestampOld;

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

      resetVideoBuffer(0, 0);

      // 是否是第一帧
      boolean isFirst = true;

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
          int dataType;
          byte[] data;
          int begin;
          int len;
          int payloadType = streamData.getPayloadType();
          int timestamp = streamData.getTimeStamp();

          if (streamType == VmType.STREAM_TYPE_AUDIO) {  // 如果是音频，则不需要拼包和分包
            dataType = DATA_TYPE_AUDIO;
            data = streamData.getBuffer();
            begin = 0;
            len = streamData.getBuffer().length;

            sendData(dataType, payloadType, timestamp, data, begin, len);
          } else if (streamType == VmType.STREAM_TYPE_VIDEO) {  // 视频数据
            int packetLen = streamData.getBuffer().length;
            streamBufUsed = 0;

            while (packetLen - streamBufUsed > 0) {
              // 分帧
              int nalLen = mergeBuffer(videoBuf, videoBufUsed, streamData.getBuffer(),
                  streamBufUsed, packetLen - streamBufUsed);
              videoBufUsed += nalLen;
              streamBufUsed += nalLen;

              // 分出完整帧
              if (trans == 1) {
                // 第一帧的数据可能只会有部分内容，通常将它抛弃比较合理
                if (isFirst) {
                  isFirst = false;
                  resetVideoBuffer(payloadType, timestamp);
                  break;
                } else {
                  if ((videoBuf[4] & 0x1F) == 7) {  // sps
                    dataType = DATA_TYPE_VIDEO_SPS;
                  } else if ((videoBuf[4] & 0x1F) == 8) {  // pps
                    dataType = DATA_TYPE_VIDEO_PPS;
                  } else if (videoBuf[4] == 0x65) {  // I帧
                    dataType = DATA_TYPE_VIDEO_IFRAME;
                  } else {  // 否则都当做P帧
                    dataType = DATA_TYPE_VIDEO_PFRAME;
                  }
                  data = videoBuf;
                  begin = 0;
                  len = videoBufUsed - 4;  // 最后会多4个字节的0x00 00 00 01

                  sendData(dataType, payloadType, timestamp, data, begin, len);
                }
              }
            }

          } else {
            Log.e(TAG, "暂时不支持[" + streamType + "]类型码流的解码！");
          }

        }
      } catch (InterruptedException e) {

      }
      Log.i(TAG, "帧数据处理线程结束...");
    }

    private void sendData(int dataType, int payloadType, int timestamp, byte[] data, int begin,
                          int len) {
      // 需要发送的数据不为空时，需要发送
      if (data != null) {
        // 解析出es元数据后，看线程是否需要使用该数据，发送数据到相应的线程
        if (playThread != null) {
          EsStreamData esStreamData = new EsStreamData(dataType, payloadTypeOld, timestampOld, data,
              begin, len);
          playThread.addBuffer(esStreamData);
        }
        if (recordThread != null) {
          EsStreamData esStreamData = new EsStreamData(dataType, payloadTypeOld, timestampOld, data,
              begin, len);
          recordThread.addBuffer(esStreamData);
        }
        // 发送完之后再重置视频缓存
        resetVideoBuffer(payloadType, timestamp);
      }
    }

    private void resetVideoBuffer(int payloadType, int timestamp) {
      trans = 0xFFFFFFFF;

      videoBuf[0] = 0;
      videoBuf[1] = 0;
      videoBuf[2] = 0;
      videoBuf[3] = 1;

      videoBufUsed = 4;

      payloadTypeOld = payloadType;
      timestampOld = timestamp;
    }

    // 根据0x00 00 00 01 来进行分段
    private final int mergeBuffer(byte[] NalBuf, int NalBufUsed, byte[] SockBuf, int SockBufUsed,
                                  int SockRemain) {
      int i = 0;

      if (true) {
        byte Temp;
        for (; i < SockRemain; ++i) {
          Temp = SockBuf[i + SockBufUsed];
          NalBuf[i + NalBufUsed] = Temp;

          trans <<= 8;
          trans |= Temp;

          if (trans == 1) {
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
          trans = 1;
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
    private SurfaceHolder surfaceHolder;
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

    // 音频相关
    private AudioTrack audioDecoder;

    // oepngles渲染
    private boolean useOpengles = false;

    // 使用opengles时，从c++层返回的YUV数据
    private byte[] yBuffer;
    private byte[] uBuffer;
    private byte[] vBuffer;

    private FrameConfHolder frameConfHolder = new FrameConfHolder();

    private boolean showed = false;

    private Display displayThread;

    private long timestamp;

    // 播放数据
    private BlockingBuffer playBuffer = new BlockingBuffer(BUFFER_MAX_SIZE, BUFFER_WARNING_SIZE);

    // surface是否被创建
    private boolean surfaceCreated = false;
    private int surfaceWidth;
    private int surfaceHeight;

    public PlayThread(int decodeType, boolean closeOpengles, boolean closeSmooth, SurfaceHolder
        surfaceHolder) {
      this.decodeType = decodeType;
      this.closeOpengles = closeOpengles;
      this.closeSmooth = closeSmooth;
      this.surfaceHolder = surfaceHolder;
      if (this.surfaceHolder != null) {
        this.surfaceCreated = this.surfaceHolder.getSurface().isValid();
        this.surfaceWidth = surfaceHolder.getSurfaceFrame().width();
        this.surfaceHeight = surfaceHolder.getSurfaceFrame().height();
      }
    }

    public synchronized boolean screenshot(String fileName) {
      if (lastSpsBuffer == null || lastPpsBuffer == null || lastIFrameBuffer == null) {
        return false;
      }

      FileOutputStream fileOutputStream;
      try {
        fileOutputStream = new FileOutputStream(fileName);
      } catch (FileNotFoundException e) {
        Log.e(TAG, StringUtil.getStackTraceAsString(e));
        return false;
      }

      if (fileOutputStream == null) {
        return false;
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
        ret = DecodeNalu2RGB(screenshotDecoderHandle, lastPFrameBuffer, lastPFrameBuffer.length,
            rgbData, frameConfHolder);
      }
      DecoderUninit(screenshotDecoderHandle);

//      Log.e(TAG, "ret=" + ret);
      if (ret <= 0) {
        return false;
      }

      ByteBuffer buffer = ByteBuffer.wrap(rgbData, 0, size);
      Bitmap screenBit = Bitmap.createBitmap(w, h, Bitmap.Config.RGB_565);
      buffer.rewind();
      screenBit.copyPixelsFromBuffer(buffer);

      screenBit.compress(Bitmap.CompressFormat.JPEG, 100, fileOutputStream);
      try {
        fileOutputStream.flush();
        fileOutputStream.close();
      } catch (IOException e) {
        Log.e(TAG, StringUtil.getStackTraceAsString(e));
        return false;
      }

      return true;
    }

    private synchronized void createAudioDecoder() {
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

    private synchronized void releaseAudioDecoder() {
      if (audioDecoder == null) {
        return;
      }
      audioDecoder.release();
      audioDecoder = null;
    }

    private synchronized void createDecoder(int payloadType) {
      currentPayloadType = payloadType;
      // 窗口不存在则直接跳过
      if (!surfaceCreated) {
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
                useOpengles = true;
              } else {
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
              mediaCodecDecoder.configure(mediaFormat, surfaceHolder.getSurface(), null, 0);
//              mediaCodecDecoder.configure(mediaFormat, null, null, 0);  //
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

    private synchronized void releaseDecoder() {
//      Log.e(TAG, "releaseDecoder");
      // 窗口不存在则直接跳过
//      if (!surfaceCreated) {
//        return;
//      }
      if (decodeType == VmType.DECODE_TYPE_SOFTWARE) {
        if (decoderHandle != 0) {
          Log.e(TAG, "DecoderUninit " + decoderHandle);
          DecoderUninit(decoderHandle);
          decoderHandle = 0;
        }
      } else {
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
          mediaCodecDecoder.release();
//          Log.e(TAG, "mediaCodecDecoder.release()");
          mediaCodecDecoder = null;
        }
      }

//      if (displayThread != null) {
//        displayThread.shutDown();
//        displayThread = null;
//      }
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
    }

    //    FileOutputStream fileOutputStream;
    @Override
    public void run() {
      Log.i(TAG, "解码线程开始...");

      try {
        while (isRunning && !isInterrupted()) {
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

          // 如果是音频的话，则只需要执行这段代码即可
          if (dataType == DATA_TYPE_AUDIO) {
            createAudioDecoder();
            audioDecoder.write(esStreamData.getData(), 0, esStreamData.getData().length);
            continue;
          }

          // 如果队列中大于100帧没有处理，则做丢帧处理，只对p帧丢包，别的包不可以丢
          if (playBuffer.size() > 100 && dataType == DATA_TYPE_VIDEO_PFRAME) {
            continue;
          }

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

          // 测试不给sps和pps时是无法解码的
//          if (dataType == DATA_TYPE_VIDEO_SPS || dataType == DATA_TYPE_VIDEO_PPS) {
//            continue;
//          }

          byte[] data = esStreamData.getData();

          // 保存最新的I帧和sps、pps数据供截图使用
          if (dataType == DATA_TYPE_VIDEO_SPS) {
            lastSpsBuffer = new byte[data.length];
            System.arraycopy(data, 0, lastSpsBuffer, 0, data.length);
//            Log.e(TAG, "sps");
          } else if (dataType == DATA_TYPE_VIDEO_PPS) {
            lastPpsBuffer = new byte[data.length];
            System.arraycopy(data, 0, lastPpsBuffer, 0, data.length);
//            Log.e(TAG, "pps");
          } else if (dataType == DATA_TYPE_VIDEO_IFRAME) {
            tmpIFrameBuffer = new byte[data.length];
            System.arraycopy(data, 0, tmpIFrameBuffer, 0, data.length);
//            lastIFrameBuffer = new byte[data.length];
//            System.arraycopy(data, 0, lastIFrameBuffer, 0, data.length);
            isFistPFrame = true;
//            Log.e(TAG, "IFrame");
          } else if (dataType == DATA_TYPE_VIDEO_PFRAME && isFistPFrame && tmpIFrameBuffer !=
              null) {
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
              canShow = DecodeNalu2YUV(decoderHandle, data, data.length, yBuffer, uBuffer,
                  vBuffer, frameConfHolder);
              if (canShow >= 0) {
                int tmpWidth = frameConfHolder.getWidth() > 0 ? frameConfHolder.getWidth() : w;
                int tmpHeight = frameConfHolder.getHeight() > 0 ? frameConfHolder.getHeight() : h;
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
                  canShow = DecodeNalu2YUV(decoderHandle, data, data.length, yBuffer, uBuffer,
                      vBuffer, frameConfHolder);
                }
                // 可以显示
                if (canShow > 0 && displayThread != null) {
                  // 异步处理
                  YUVFrameData yuvFrameData = new YUVFrameData(yBuffer, uBuffer, vBuffer, w, h);
                  displayThread.addBuffer(yuvFrameData);
                }
              }
            } else {
              canShow = DecodeNalu2RGB(decoderHandle, data, data.length, pixelBuffer,
                  frameConfHolder);
//              Log.e(TAG, "canShow=" + canShow + " , dataType=" + dataType);
              if (canShow >= 0) {
                int tmpWidth = frameConfHolder.getWidth() > 0 ? frameConfHolder.getWidth() : w;
                int tmpHeight = frameConfHolder.getHeight() > 0 ? frameConfHolder.getHeight() : h;
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
                  canShow = DecodeNalu2RGB(decoderHandle, data, data.length, pixelBuffer,
                      frameConfHolder);
                }
                // 可以显示
                if (canShow > 0) {
                  // 异步处理
                  // draw(surfaceHolder);
                  if (displayThread != null) {
                    RGBFrameData rgbFrameData = new RGBFrameData(pixelBuffer, w, h);
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
              releaseAudioDecoder();
            }
          }
        }
      } catch (InterruptedException e) {
        Log.w(TAG, "解码线程中断!");
        Log.w(TAG, StringUtil.getStackTraceAsString(e));
      }
      releaseDecoder();
      releaseAudioDecoder();
      Log.i(TAG, "解码线程结束...");
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

    @Override
    protected void finalize() throws Throwable {
      super.finalize();
//      if (surfaceHolder != null) {
//        surfaceHolder.removeCallback(this);
//      }
    }

    public void pauseDisplay() {
      if (displayThread != null) {
        displayThread.pauseDisplay();
      }
    }

    public void continueDisplay() {
      if (displayThread != null) {
        displayThread.continueDisplay();
      }
    }

    private class Display extends Thread implements SurfaceHolder.Callback {
      private long renderHandle;
      private int width;
      private int height;
      private Bitmap videoBitmap;
      private ByteBuffer rgbBuffer;
      private long lastDisplay;
      private boolean isPause = false;

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

      private void drawRGB(SurfaceHolder surfaceHolder, byte[] frameData, int width, int height) {
        if (this.width != width || this.height != height) {
          this.width = width;
          this.height = height;
          videoBitmap = Bitmap.createBitmap(this.width, this.height, Bitmap.Config.RGB_565);
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
            int displayWidth = surfaceWidth > 0 ? surfaceWidth : surfaceHolder.getSurfaceFrame()
                .width();
            int displayHeight = surfaceHeight > 0 ? surfaceHeight : surfaceHolder.getSurfaceFrame
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

      @Override
      protected void finalize() throws Throwable {
        super.finalize();
        // releaseRender();  // 这里不在同一个线程里执行是无法释放的
      }

      private void createRender() {
        if (useOpengles && renderHandle == 0 && surfaceCreated) {
          renderHandle = RenderInit(surfaceHolder.getSurface());
        }
        // 同步大小
        if (renderHandle != 0) {
          RenderChangeSize(renderHandle, surfaceWidth, surfaceHeight);
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
                drawRGB(surfaceHolder, rgbFrameData.getData(), rgbFrameData.getWidth(), rgbFrameData
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

                DrawYUV(renderHandle, yData, yData.length, uData, uData.length, vData, vData
                    .length, yuvFrameData.getWidth(), yuvFrameData.getHeight());
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
                  int outIndex = mediaCodecDecoder.dequeueOutputBuffer(info, 0);
                  if (outIndex >= 0) {
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
            while (isRunning && !this.isInterrupted() && display && (frameBuffer.size() < 5) &&
                getInputCount() < 5 && lastDisplay > 0) {
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

      public void pauseDisplay() {
        isPause = true;
      }

      public void continueDisplay() {
        isPause = false;
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
    private BlockingBuffer recordBuffer = new BlockingBuffer(BUFFER_MAX_SIZE, BUFFER_WARNING_SIZE);
    private FlvSave flvSave;
    private FileOutputStream fileOutputStream;
    private byte[] sps;  // 需要写入的sps

    private RecordThread() {

    }

    public RecordThread(FileOutputStream fileOutputStream) {
      this.fileOutputStream = fileOutputStream;
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
      if (fileOutputStream == null) {
        return;
      }
      Log.i(TAG, "录像线程开始...");

      boolean findSps = false;  // 是否找到sps
      boolean confWrited = false;  // 配置是否写入

      if (flvSave == null) {
        flvSave = new FlvSave(fileOutputStream);
        if (!flvSave.writeStart()) {  // 如果写头失败的话，返回
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
            flvSave.writeConfiguretion(sps, 0, sps.length, data, 4,
                data.length - 4, esStreamData.getTimestamp());
            confWrited = true;
          } else if (confWrited) {
            flvSave.writeNalu(dataType == DATA_TYPE_VIDEO_IFRAME, data, 4, data.length - 4,
                esStreamData.getTimestamp());
          }
        }
      } catch (Exception e) {
        Log.w(TAG, StringUtil.getStackTraceAsString(e));
      }
      if (flvSave != null) {
        flvSave.save();
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

    public EsStreamData(int dataType, int payloadType, int timestamp, byte[] data, int begin, int
        len) {
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

  private static native long DecoderInit(int payloadType);

  private static native void DecoderUninit(long decoderHandle);

  private static native int DecodeNalu2RGB(long decoderHandle, byte[] inData, int inLen, byte[]
      outData, FrameConfHolder frameConfHolder);

  private static native int DecodeNalu2YUV(long decoderHandle, byte[] inData, int inLen, byte[]
      yData, byte[] uData, byte[] vData, FrameConfHolder frameConfHolder);

  private static native int GetFrameWidth(long decoderHandle);

  private static native int GetFrameHeight(long decoderHandle);

  private static native long RenderInit(Surface surface);

  private static native void RenderUninit(long renderHandle);

  private static native void RenderChangeSize(long renderHandle, int width, int height);

  private static native int DrawYUV(long renderHandle, byte[] yData, int yLen, byte[] uData, int
      uLen, byte[] vData, int vLen, int width, int height);

  private static native int OfflineScreenRendering(long renderHandle, byte[] yData, int yLen,
                                                   byte[] uData, int uLen, byte[] vData, int
                                                       vLen, int width, int height, byte[]
                                                       outData);
}

