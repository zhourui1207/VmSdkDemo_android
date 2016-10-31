package com.jxlianlian.masdk.core;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.media.MediaCodec;
import android.media.MediaFormat;
import android.os.AsyncTask;
import android.util.Log;
import android.view.SurfaceHolder;

import com.jxlianlian.masdk.VmType;
import com.jxlianlian.masdk.util.FlvSave;
import com.jxlianlian.masdk.util.StringUtil;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.nio.ByteBuffer;

/**
 * Created by zhourui on 16/10/28.
 * 解码器，包含软硬解视频，截图，实时录像，打开关闭音频等功能
 */

public class Decoder {
  private final String TAG = "Decoder";

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

  private boolean isRunning = false;

  private HandleFrameThread handleFrameThread;
  private PlayThread playThread;
  private RecordThread recordThread;

  private Decoder() {

  }

  public Decoder(int decodeType) {
    this.decodeType = decodeType;
  }

  public Decoder(int decodeType, boolean autoPlay, SurfaceHolder surfaceHolder) {
    this.decodeType = decodeType;
    this.surfaceHolder = surfaceHolder;
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
        playThread = new PlayThread(decodeType, surfaceHolder);
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
    if (handleFrameThread != null && recordThread == null) {
      isRunning = false;
      handleFrameThread.shutDown();
      handleFrameThread = null;
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
      return streamBuffer.addObject(object);
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
      interrupt();
      try {
        join();
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
    private SurfaceHolder surfaceHolder;
    private boolean isTv = false;

    private MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
    private int w = 1920;
    private int h = 1080;
    private String mime = "video/avc";
    private MediaFormat mediaFormat = MediaFormat.createVideoFormat(mime, w, h);
    private MediaCodec mediaCodecDecoder;  //硬解码器
    private ByteBuffer[] inputBuffers;

    private AudioTrack audioDecoder;

    boolean showed = false;

    private Display displayThread;

    private long timestamp;

    // 播放数据
    private BlockingBuffer playBuffer = new BlockingBuffer(BUFFER_MAX_SIZE, BUFFER_WARNING_SIZE);

    private PlayThread() {

    }

    public PlayThread(int decodeType, SurfaceHolder surfaceHolder) {
      this.decodeType = decodeType;
      this.surfaceHolder = surfaceHolder;
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

    private synchronized void createDecoder() {
      if (decodeType == VmType.DECODE_TYPE_SOFTWARE) {

      } else {
        if (mediaCodecDecoder == null && isRunning) {
          try {
            if (surfaceHolder != null && surfaceHolder.getSurface() != null) {
              Log.e(TAG, "createDecoderByType");
              mediaCodecDecoder = MediaCodec.createDecoderByType(mime);
              Log.e(TAG, "configure");
              mediaCodecDecoder.configure(mediaFormat, surfaceHolder.getSurface(), null, 0);
              Log.e(TAG, "start");
              mediaCodecDecoder.start();

              Log.e(TAG, "getInputBuffers");
              inputBuffers = mediaCodecDecoder.getInputBuffers();

              if (displayThread == null) {
                displayThread = new Display();
              }
              displayThread.start();
            }
          } catch (Exception e) {
            Log.e(TAG, StringUtil.getStackTraceAsString(e));
          }
        }
      }
    }

    private synchronized void releaseDecoder() {
      if (decodeType == VmType.DECODE_TYPE_SOFTWARE) {

      } else {
        if (mediaCodecDecoder != null && isRunning) {
          try {
            if (displayThread != null) {
              displayThread.interrupt();
              displayThread = null;
            }
            mediaCodecDecoder.stop();
            mediaCodecDecoder.release();
          } catch (Exception e) {
            Log.e(TAG, StringUtil.getStackTraceAsString(e));
          }
          mediaCodecDecoder = null;
        }
      }
    }

    public boolean addBuffer(Object object) {
      return playBuffer.addObject(object);
    }

    public void shutDown() {
      try {
        interrupt();
        join();
      } catch (Exception e) {
        Log.e(TAG, StringUtil.getStackTraceAsString(e));
      }
    }

    @Override
    public void run() {
      Log.i(TAG, "解码线程开始...");

      while (isRunning && !isInterrupted()) {
        try {
          EsStreamData esStreamData = (EsStreamData) playBuffer.removeObjectBlocking();
          if (!isRunning || esStreamData == null) {
            break;
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

          createDecoder();
          if (mediaCodecDecoder == null) {
            continue;
          }

          if (decodeType == VmType.DECODE_TYPE_SOFTWARE) {  // 软件解码

          } else {  // 智能解码或者硬件解码
            int inputBufferIndex;
            if (isTv) {
              inputBufferIndex = mediaCodecDecoder.dequeueInputBuffer(1000);
            } else {
              inputBufferIndex = mediaCodecDecoder.dequeueInputBuffer(80000);
            }

            if (inputBufferIndex >= 0) {
              ByteBuffer inputBuffer = inputBuffers[inputBufferIndex];
              inputBuffer.clear();
              byte[] data = esStreamData.getData();
              inputBuffer.put(data, 0, data.length);

              int flags;
              switch (dataType) {
                case DATA_TYPE_VIDEO_SPS:
                case DATA_TYPE_VIDEO_PPS:
                  flags = MediaCodec.BUFFER_FLAG_CODEC_CONFIG;
                  break;
                default:
                  flags = MediaCodec.BUFFER_FLAG_KEY_FRAME;
                  timestamp = (timestamp + 40000) % Long.MAX_VALUE;
                  break;
              }
              mediaCodecDecoder.queueInputBuffer(inputBufferIndex, 0,
                  data.length, timestamp, flags);

            }
          }
        } catch (Exception e) {
          Log.w(TAG, StringUtil.getStackTraceAsString(e));
          releaseDecoder();
          releaseAudioDecoder();
        }
      }
      Log.i(TAG, "解码线程结束...");
    }

    private class Display extends Thread {
      @Override
      public void run() {
        Log.i(TAG, "显示线程开始...");
        while (isRunning && !isInterrupted()) {
          if (mediaCodecDecoder != null && showed) {
            try {
              int outIndex = mediaCodecDecoder.dequeueOutputBuffer(info, 40000);
              if (outIndex >= 0) {
                if (info.size > 0) {
                  mediaCodecDecoder.releaseOutputBuffer(outIndex, true);
                } else {
                  mediaCodecDecoder.releaseOutputBuffer(outIndex, false);
                }
              }
            } catch (Exception e) {
              Log.w(TAG, StringUtil.getStackTraceAsString(e));
              releaseDecoder();
            }
          }
        }
        Log.i(TAG, "显示线程结束...");
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
      interrupt();
      try {
        join();
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

  // 截图异步任务
  private class ScreenshotTask extends AsyncTask<Void, Void, Void> {

    @Override
    protected Void doInBackground(Void... voids) {
      return null;
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
}

