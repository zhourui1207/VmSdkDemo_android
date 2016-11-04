package com.jxlianlian.masdk;

import android.content.Context;
import android.os.AsyncTask;
import android.util.Log;
import android.view.SurfaceHolder;

import com.jxlianlian.masdk.core.Decoder;
import com.jxlianlian.masdk.core.StreamData;

/**
 * Created by zhourui on 16/10/28.
 * sdk解码接口，解码接口依赖通信接口
 */

public class VmPlayer {
  private final String TAG = "VmPlayer";

  // 播放状态侦听器
  public interface PlayStatusListener {
    void onStatusChanged(int currentStatus);  // 播放状态改变
  }

  // 码流状态侦听器
  public interface StreamStatusListener {
    void onVideoStreamStatus(boolean isConnected);

    void onAudioStreamStatus(boolean isConnected);
  }

  private int currentStatus = VmType.PLAY_STATUS_NONE;
  private PlayStatusListener playStatusListener;
  private StreamStatusListener streamStatusListener;

  private int errorCode;

  private int playMode = VmType.PLAY_MODE_NONE;  // 播放模式，默认是－1; 0:实时视频； 1:录像回放

  private String fdId = "";
  private int channelId = 1;
  private boolean isSub = true;
  private boolean isCenter = true;
  private int beginTime;
  private int endTime;

  private int decodeType;
  private boolean openAudio = false;

  private SurfaceHolder surfaceHolder;
  private Context context;

  private int monitorId;
  private int videoStreamId;
  private int audioStreamId;

  private OpenStreamTask openStreamTask;
  private StartStreamTask startStreamTask;

  private Decoder videoDecoder;
  private Decoder audioDecoder;

  /**
   * 返回当前错误码，若播放状态变回PLAY_MODE_NONE时，调用此接口获取最后的错误码
   *
   * @return
   */
  public int getErrorCode() {
    return errorCode;
  }

  /**
   * 设置播放状态侦听器
   *
   * @param playStatusListener
   */
  public void setPlayStatusListener(PlayStatusListener playStatusListener) {
    this.playStatusListener = playStatusListener;
  }

  /**
   * 取消播放状态侦听器
   */
  public void removePlayStatusListener() {
    playStatusListener = null;
  }

  /**
   * 设置码流状态侦听器
   *
   * @param statusStatusListener
   */
  public void setStatusStatusListener(StreamStatusListener statusStatusListener) {
    this.streamStatusListener = statusStatusListener;
  }

  /**
   * 取消码流状态侦听器
   */
  public void removeStreamStatusListener() {
    streamStatusListener = null;
  }

  /**
   * 播放实时流，此函数为异步接口
   *
   * @param fdId 设备id
   * @param channelId 通道号
   * @param isSub 是否子码流
   * @param decodeType 解码类型
   * @param openAudio 是否打开音频
   * @param surfaceHolder 播放页面
   * @param context 播放页面上下文
   * @return true：开始执行播放任务；false：未执行播放任务（通常是正在播放录像原因）
   */
  public synchronized boolean startRealplay(String fdId, int channelId, boolean isSub, int
      decodeType, boolean openAudio, SurfaceHolder surfaceHolder, Context context) {
    if (playMode == VmType.PLAY_MODE_PLAYBACK) {
      return false;
    }
    playMode = VmType.PLAY_MODE_REALPLAY;
    this.fdId = fdId;
    this.channelId = channelId;
    this.isSub = isSub;
    this.decodeType = decodeType;
    this.openAudio = openAudio;
    this.surfaceHolder = surfaceHolder;
    this.context = context;

    doOpenStreamTask();
    return true;
  }

  /**
   * 播放实时流，此函数为异步接口
   *
   * @param videoAddr 视频地址
   * @param videoPort 视频端口
   * @param audioAddr 音频地址
   * @param audioPort 音频端口
   * @param decodeType 解码类型
   * @param openAudio 是否打开音频
   * @param surfaceHolder 播放页面
   * @param context 播放页面上下文
   * @return true：开始执行播放任务；false：未执行播放任务（通常是正在播放录像原因）
   */
  public synchronized boolean startRealplay(String videoAddr, int videoPort, String audioAddr,
                                            int audioPort, int
      decodeType, boolean openAudio, SurfaceHolder surfaceHolder, Context context) {
    if (playMode == VmType.PLAY_MODE_PLAYBACK) {
      return false;
    }

    playMode = VmType.PLAY_MODE_REALPLAY;
    this.decodeType = decodeType;
    this.openAudio = openAudio;
    this.surfaceHolder = surfaceHolder;
    this.context = context;

    PlayAddressHolder holder = new PlayAddressHolder();
    holder.init(0, videoAddr, videoPort, audioAddr, audioPort);
    doStartStreamTask(holder);
    return true;
  }

  /**
   * 停止播放
   */
  public synchronized void stopPlay() {
    playStatusListener = null;
    streamStatusListener = null;
    setCurrentStatus(VmType.PLAY_STATUS_NONE);
    // 下面皆为同步操作
    doCloseStreamTask();
    doStopStreamTask();
    doStopDecodeTask();
    return;
  }

  /**
   * 打开声音
   *
   * @return
   */
  public synchronized boolean openAudio() {
    if (audioDecoder != null) {
      return audioDecoder.startPlay();
    }
    return false;
  }

  /**
   * 关闭声音
   */
  public synchronized void closeAudio() {
    if (audioDecoder != null) {
      audioDecoder.stopPlay();
    }
  }

  /**
   * 开始实时录像，若目录不存在，需要客户自己去创建目录
   *
   * @param file 生成文件的全路径 如 /sdcard/MVSS/LocalRecord/record2014-12-07.flv
   * @return
   */
  public synchronized boolean startRecord(String file) {
    if (videoDecoder != null) {
      return videoDecoder.startRecord(file);
    }
    return false;
  }

  /**
   * 停止实时录像
   */
  public synchronized void stopRecord() {
    if (videoDecoder != null) {
      videoDecoder.stopRecord();
    }
  }

  @Override
  protected void finalize() throws Throwable {
    super.finalize();
    stopPlay();
  }

  private void setCurrentStatus(int status) {
    if (currentStatus == status) {
      return;
    }
    currentStatus = status;
    if (playStatusListener != null) {
      playStatusListener.onStatusChanged(currentStatus);
    }
  }

  private void doOpenStreamTask() {
    if (openStreamTask != null) {
      openStreamTask.cancel(true);
    }
    openStreamTask = new OpenStreamTask();
    openStreamTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
  }

  private void doCloseStreamTask() {
    if (monitorId == 0) {
      return;
    }
    switch (playMode) {
      case VmType.PLAY_MODE_REALPLAY:
        VmNet.closeRealplayStream(monitorId);
        break;
      case VmType.PLAY_MODE_PLAYBACK:
        VmNet.closePlaybackStream(monitorId, isCenter);
        break;
      default:
        Log.e(TAG, "关闭码流失败！当前播放模式为[" + playMode + "]，未定义的值！");
        break;
    }
  }

  private void doStartStreamTask(PlayAddressHolder holder) {
    if (startStreamTask != null) {
      startStreamTask.cancel(true);
    }
    startStreamTask = new StartStreamTask();
    startStreamTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, holder);
  }

  private void doStopStreamTask() {
    VmNet.stopStram(videoStreamId);
    VmNet.stopStram(audioStreamId);
  }

  private void doStopDecodeTask() {
    if (videoDecoder != null) {
      videoDecoder.stopPlay();
      videoDecoder = null;
    }

    if (audioDecoder != null) {
      audioDecoder.stopPlay();
      audioDecoder = null;
    }
  }

  private class OpenStreamTask extends AsyncTask<Void, Void, Void> {

    @Override
    protected Void doInBackground(Void... voids) {
      PlayAddressHolder holder = new PlayAddressHolder();
      setCurrentStatus(VmType.PLAY_STATUS_OPENSTREAMING);
      errorCode = ErrorCode.NO_EXECUTE;
      switch (playMode) {
        case VmType.PLAY_MODE_REALPLAY:
          errorCode = VmNet.openRealplayStream(fdId, channelId, isSub, holder);
          break;
        case VmType.PLAY_MODE_PLAYBACK:
          errorCode = VmNet.openPlaybackStream(fdId, channelId, isCenter, beginTime, endTime,
              holder);
          break;
        default:
          Log.e(TAG, "打开码流失败！当前播放模式为[" + playMode + "]，未定义的值！");
          break;
      }
      if (errorCode == ErrorCode.ERR_CODE_OK) {
        monitorId = holder.getMonitorId();
        // 开始取流
        doStartStreamTask(holder);
      } else {
        setCurrentStatus(VmType.PLAY_STATUS_NONE);
      }
      return null;
    }

    @Override
    protected void onPostExecute(Void aVoid) {
      openStreamTask = null;
    }
  }

  private class StartStreamTask extends AsyncTask<PlayAddressHolder, Void, Void> {

    @Override
    protected Void doInBackground(PlayAddressHolder... playAddressHolders) {
      PlayAddressHolder holder = playAddressHolders[0];
      setCurrentStatus(VmType.PLAY_STATUS_GETSTREAMING);

      errorCode = ErrorCode.NO_EXECUTE;
      StreamIdHolder streamIdHolder = new StreamIdHolder();
      errorCode = VmNet.startStream(holder.getVideoAddr(), holder.getVideoPort(), new
          VideoStreamCallbackI(), streamIdHolder);
      Log.e(TAG, "errorCode" + errorCode);
      if (errorCode == ErrorCode.ERR_CODE_OK) {
        videoStreamId = streamIdHolder.getStreamId();
        // 视频开始解码
        if (videoDecoder == null) {
          videoDecoder = new Decoder(decodeType, true, surfaceHolder, context);
        } else {
          videoDecoder.startPlay();
        }
      } else {
        doCloseStreamTask();
        setCurrentStatus(VmType.PLAY_STATUS_NONE);
      }

      // 若音频取流失败，则不做任何处理 todo:现在的音频类型是有问题的，因为音频的头居然使用的是视频的头
      errorCode = VmNet.startStream(holder.getAudioAddr(), holder.getAudioPort(), new
          AudioStreamCallbackI(), streamIdHolder);
      if (errorCode == ErrorCode.ERR_CODE_OK) {
        audioStreamId = streamIdHolder.getStreamId();
        // 音频开始解码
        if (audioDecoder == null) {
          audioDecoder = new Decoder(decodeType, openAudio, surfaceHolder, context);
        } else if (openAudio) {
          audioDecoder.startPlay();
        }
      }
      return null;
    }

    @Override
    protected void onPostExecute(Void aVoid) {
      startStreamTask = null;
    }
  }

  private class VideoStreamCallbackI implements VmNet.StreamCallback {

    @Override
    public void onStreamConnectStatus(int streamId, boolean isConnected) {
      if (streamStatusListener != null) {
        streamStatusListener.onVideoStreamStatus(isConnected);
      }
    }

    @Override
    public void onReceiveStream(int streamId, int streamType, int payloadType, byte[] buffer, int
        timeStamp, int seqNumber, boolean isMark) {

//      Log.e(TAG, "!!");
      if (videoDecoder != null) {
        videoDecoder.addBuffer(new StreamData(streamId, VmType.STREAM_TYPE_VIDEO, payloadType,
            buffer,
            timeStamp, seqNumber, isMark));
      }
    }
  }

  private class AudioStreamCallbackI implements VmNet.StreamCallback {

    @Override
    public void onStreamConnectStatus(int streamId, boolean isConnected) {
      if (streamStatusListener != null) {
        streamStatusListener.onAudioStreamStatus(isConnected);
      }
    }

    @Override
    public void onReceiveStream(int streamId, int streamType, int payloadType, byte[] buffer, int
        timeStamp, int seqNumber, boolean isMark) {

      if (audioDecoder != null) {
        audioDecoder.addBuffer(new StreamData(streamId, VmType.STREAM_TYPE_AUDIO, payloadType,
            buffer,
            timeStamp, seqNumber, isMark));
      }
    }
  }
}
