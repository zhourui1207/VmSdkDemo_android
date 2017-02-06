package com.jxll.vmsdk;

import android.content.Context;
import android.os.AsyncTask;
import android.util.Log;
import android.view.SurfaceHolder;

import com.jxll.vmsdk.core.Decoder;
import com.jxll.vmsdk.core.StreamData;

/**
 * Created by zhourui on 16/10/28.
 * sdk解码接口，解码接口依赖通信接口
 */

public class VmPlayer {
    private final String TAG = "VmPlayer";
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
    private boolean closeOpengles = false;
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
     * 获取是否是中心录像回放
     *
     * @return
     */
    public boolean isCenter() {
        return isCenter;
    }

    /**
     * 获取监控ID，客户可以在视频播放后，使用此监控id调用网络sdk的其他接口
     *
     * @return
     */
    public int getMonitorId() {
        return monitorId;
    }

    /**
     * 返回当前错误码，若播放状态变回PLAY_MODE_NONE时，调用此接口获取最后的错误码
     *
     * @return
     */
    public int getErrorCode() {
        return errorCode;
    }

    /**
     * 获取播放模式
     *
     * @return
     */
    public int getPlayMode() {
        return playMode;
    }

    /**
     * 获取当前状态
     *
     * @return
     */
    public int getCurrentStatus() {
        return currentStatus;
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
     * @param fdId          设备id
     * @param channelId     通道号
     * @param isSub         是否子码流
     * @param decodeType    解码类型
     * @param closeOpengles 是否强制关闭opengles
     * @param openAudio     是否打开音频
     * @param surfaceHolder 播放页面
     * @param context       播放页面上下文
     * @return true：开始执行播放任务；false：未执行播放任务（通常是正在播放录像原因）
     */
    public synchronized boolean startRealplay(String fdId, int channelId, boolean isSub, int
            decodeType, boolean closeOpengles, boolean openAudio, SurfaceHolder surfaceHolder, Context
                                                      context) {
//    if (playMode == VmType.PLAY_MODE_PLAYBACK) {
//      return false;
//    }

        // 如果之前已经在播放，那么需要先停止
        if (currentStatus != VmType.PLAY_STATUS_NONE) {
            stopPlay();
        }
        setCurrentStatus(VmType.PLAY_STATUS_BUSY);

        playMode = VmType.PLAY_MODE_REALPLAY;
        this.fdId = fdId;
        this.channelId = channelId;
        this.isSub = isSub;
        this.decodeType = decodeType;
        this.closeOpengles = closeOpengles;
        this.openAudio = openAudio;
        this.surfaceHolder = surfaceHolder;
        this.context = context;

        doOpenStreamTask();
        return true;
    }

    /**
     * 播放实时流，此函数为异步接口
     *
     * @param videoAddr     视频地址
     * @param videoPort     视频端口
     * @param audioAddr     音频地址
     * @param audioPort     音频端口
     * @param decodeType    解码类型
     * @param openAudio     是否打开音频
     * @param surfaceHolder 播放页面
     * @param context       播放页面上下文
     * @return true：开始执行播放任务；false：未执行播放任务（通常是正在播放录像原因）
     */
    public synchronized boolean startRealplay(String videoAddr, int videoPort, String audioAddr,
                                              int audioPort, int decodeType, boolean closeOpengles,
                                              boolean openAudio, SurfaceHolder
                                                      surfaceHolder, Context context) {
//    if (playMode == VmType.PLAY_MODE_PLAYBACK) {
//      return false;
//    }

        // 如果之前已经在播放，那么需要先停止
        if (currentStatus != VmType.PLAY_STATUS_NONE) {
            stopPlay();
        }
        setCurrentStatus(VmType.PLAY_STATUS_BUSY);

        playMode = VmType.PLAY_MODE_REALPLAY;
        this.decodeType = decodeType;
        this.closeOpengles = closeOpengles;
        this.openAudio = openAudio;
        this.surfaceHolder = surfaceHolder;
        this.context = context;

        PlayAddressHolder holder = new PlayAddressHolder();
        holder.init(0, videoAddr, videoPort, audioAddr, audioPort);
        doStartStreamTask(holder);
        return true;
    }

    /**
     * 录像回放，此函数为异步接口
     *
     * @param fdId          设备id
     * @param channelId     通道号
     * @param isCenter      是否中心录像
     * @param beginTime     录像起始时间
     * @param endTime       录像截止时间
     * @param decodeType    解码类型
     * @param closeOpengles 是否强制关闭opengles
     * @param openAudio     是否打开音频
     * @param surfaceHolder 播放页面
     * @param context       播放页面上下文
     * @return true：开始执行播放任务；false：未执行播放任务（通常是正在播放实时预览原因）
     */
    public synchronized boolean startPlayback(String fdId, int channelId, boolean isCenter, int
            beginTime, int endTime, int decodeType, boolean closeOpengles, boolean openAudio,
                                              SurfaceHolder surfaceHolder, Context context) {
//    if (playMode == VmType.PLAY_MODE_REALPLAY) {
//      return false;
//    }

        // 如果之前已经在播放，那么需要先停止
        if (currentStatus != VmType.PLAY_STATUS_NONE) {
            stopPlay();
        }
        setCurrentStatus(VmType.PLAY_STATUS_BUSY);

        playMode = VmType.PLAY_MODE_PLAYBACK;
        this.fdId = fdId;
        this.channelId = channelId;
        this.isCenter = isCenter;
        this.beginTime = beginTime;
        this.endTime = endTime;
        this.decodeType = decodeType;
        this.closeOpengles = closeOpengles;
        this.openAudio = openAudio;
        this.surfaceHolder = surfaceHolder;
        this.context = context;

        doOpenStreamTask();
        return true;
    }

    /**
     * 录像回放，此函数为异步接口
     *
     * @param videoAddr     视频地址
     * @param videoPort     视频端口
     * @param audioAddr     音频地址
     * @param audioPort     音频端口
     * @param decodeType    解码类型
     * @param openAudio     是否打开音频
     * @param surfaceHolder 播放页面
     * @param context       播放页面上下文
     * @return true：开始执行播放任务；false：未执行播放任务（通常是正在播放实时预览原因）
     */
    public synchronized boolean startPlayback(String videoAddr, int videoPort, String audioAddr,
                                              int audioPort, int decodeType, boolean closeOpengles,
                                              boolean openAudio, SurfaceHolder
                                                      surfaceHolder, Context context) {
//    if (playMode == VmType.PLAY_MODE_REALPLAY) {
//      return false;
//    }

        // 如果之前已经在播放，那么需要先停止
        if (currentStatus != VmType.PLAY_STATUS_NONE) {
            stopPlay();
        }
        setCurrentStatus(VmType.PLAY_STATUS_BUSY);

        playMode = VmType.PLAY_MODE_PLAYBACK;
        this.decodeType = decodeType;
        this.closeOpengles = closeOpengles;
        this.openAudio = openAudio;
        this.surfaceHolder = surfaceHolder;
        this.context = context;

        PlayAddressHolder holder = new PlayAddressHolder();
        holder.init(0, videoAddr, videoPort, audioAddr, audioPort);
        doStartStreamTask(holder);
        return true;
    }

    public synchronized boolean controlPlayback(String action, String param) {
        if (playMode != VmType.PLAY_MODE_PLAYBACK || currentStatus != VmType.PLAY_STATUS_PLAYING) {
            return false;
        }

        int ret = VmNet.controlPlayback(monitorId, 0, action, param);
        if (ret == ErrorCode.ERR_CODE_OK) {
            return true;
        }
        return false;
    }

    /**
     * 停止播放
     */
    public synchronized void stopPlay() {
        playStatusListener = null;
        streamStatusListener = null;
        setCurrentStatus(VmType.PLAY_STATUS_NONE);
        // 下面皆为同步操作
        doCloseStreamTask(playMode, monitorId);
        doStopStreamTask(videoStreamId, audioStreamId);
        doStopDecodeTask();
        playMode = VmType.PLAY_MODE_NONE;
        return;
    }

    /**
     * 暂停显示
     */
    public synchronized void pauseDisplay() {
        if (videoDecoder != null) {
            videoDecoder.pauseDisplay();
        }
    }

    /**
     * 继续显示
     */
    public synchronized void continueDisplay() {
        if (videoDecoder != null) {
            videoDecoder.continueDisplay();
        }
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

    /**
     * 截图，保存成jpeg/jpg格式
     *
     * @param file 截图后生成文件路径
     * @return
     */
    public synchronized boolean screenshot(String file) {
        if (videoDecoder != null) {
            return videoDecoder.screenshot(file);
        }
        return false;
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        stopPlay();
    }

    private void doOpenStreamTask() {
        if (openStreamTask != null) {
            openStreamTask.cancel(true);
        }
        openStreamTask = new OpenStreamTask();
        openStreamTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    private void doCloseStreamTask(int playMode, int monitorId) {
        if (monitorId == 0) {
            return;
        }
        switch (playMode) {
            case VmType.PLAY_MODE_REALPLAY:
                VmNet.closeRealplayStream(monitorId);
                break;
            case VmType.PLAY_MODE_PLAYBACK:
                VmNet.closePlaybackStream(monitorId);
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

    private void doStopStreamTask(int videoStreamId, int audioStreamId) {
        if (videoStreamId != 0) {
            VmNet.stopStram(videoStreamId);
        }
        if (audioStreamId != 0) {
            VmNet.stopStram(audioStreamId);
        }
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

    // 播放状态侦听器
    public interface PlayStatusListener {
        void onStatusChanged(int currentStatus);  // 播放状态改变
    }

    // 码流状态侦听器
    public interface StreamStatusListener {
        void onVideoStreamStatus(boolean isConnected);

        void onAudioStreamStatus(boolean isConnected);
    }

    private class OpenStreamTask extends AsyncTask<Void, Void, PlayAddressHolder> {

        int mErrorCode = ErrorCode.NO_EXECUTE;

        int mPlayMode;

        @Override
        protected PlayAddressHolder doInBackground(Void... voids) {
            PlayAddressHolder holder = new PlayAddressHolder();
            setCurrentStatus(VmType.PLAY_STATUS_OPENSTREAMING);
            mPlayMode = playMode;
            switch (playMode) {
                case VmType.PLAY_MODE_REALPLAY:
                    mErrorCode = VmNet.openRealplayStream(fdId, channelId, isSub, holder);
                    break;
                case VmType.PLAY_MODE_PLAYBACK:
                    mErrorCode = VmNet.openPlaybackStream(fdId, channelId, isCenter, beginTime, endTime,
                            holder);
                    break;
                default:
                    Log.e(TAG, "打开码流失败！当前播放模式为[" + playMode + "]，未定义的值！");
                    break;
            }
//      if (errorCode == ErrorCode.ERR_CODE_OK) {
//        monitorId = holder.getMonitorId();
//        // 开始取流
//        doStartStreamTask(holder);
//      } else {
//        Log.e(TAG, "打开码流失败！返回值 errorCode=" + errorCode);
//        setCurrentStatus(VmType.PLAY_STATUS_NONE);
//      }
            return holder;
        }

        @Override
        protected void onPostExecute(PlayAddressHolder holder) {
            errorCode = mErrorCode;
            if (mErrorCode == ErrorCode.ERR_CODE_OK) {
                monitorId = holder.getMonitorId();
                // 开始取流
                Log.i(TAG, "打开码流成功，开始取流...");
                doStartStreamTask(holder);
            } else {
                Log.e(TAG, "打开码流失败！返回值 mErrorCode=" + mErrorCode);
                setCurrentStatus(VmType.PLAY_STATUS_NONE);
            }
            openStreamTask = null;
        }

        @Override
        protected void onCancelled(PlayAddressHolder holder) {
            if (mErrorCode == ErrorCode.ERR_CODE_OK) {
                Log.e(TAG, "取消打开码流 mPlatMode=" + mPlayMode + ", monitorId=" + holder.getMonitorId());
                doCloseStreamTask(mPlayMode, holder.getMonitorId());
            }
        }
    }

    private class StartStreamTask extends AsyncTask<PlayAddressHolder, Void, Void> {

        int mErrorCode = ErrorCode.NO_EXECUTE;

        int mVideoStreamId;
        int mAudioStreamId;

        @Override
        protected Void doInBackground(PlayAddressHolder... playAddressHolders) {
            PlayAddressHolder holder = playAddressHolders[0];
            setCurrentStatus(VmType.PLAY_STATUS_GETSTREAMING);

            StreamIdHolder videoStreamIdHolder = new StreamIdHolder();
            mErrorCode = VmNet.startStream(holder.getVideoAddr(), holder.getVideoPort(), new
                    VideoStreamCallbackI(), videoStreamIdHolder);
            Log.w(TAG, "startStream mErrorCode=" + mErrorCode);

            if (mErrorCode == ErrorCode.ERR_CODE_OK) {
                mVideoStreamId = videoStreamIdHolder.getStreamId();
            }


//      if (holder.getAudioAddr() == null || holder.getAudioAddr().equalsIgnoreCase("") || holder
//          .getAudioPort() == 0) {
//        return null;
//      }

            StreamIdHolder audioStreamIdHolder = new StreamIdHolder();
            // 若音频取流失败，则不做任何处理 todo:现在的音频类型是有问题的，因为音频的头居然使用的是视频的头
            int ret = VmNet.startStream(holder.getAudioAddr(), holder.getAudioPort(), new
                    AudioStreamCallbackI(), audioStreamIdHolder);

            if (ret == ErrorCode.ERR_CODE_OK) {
                mAudioStreamId = audioStreamIdHolder.getStreamId();
            }

            return null;
        }

        @Override
        protected void onPostExecute(Void aVoid) {
            errorCode = mErrorCode;
            boolean closeSmooth = playMode == VmType.PLAY_MODE_PLAYBACK ? true : false;

            if (mErrorCode == ErrorCode.ERR_CODE_OK) {
                videoStreamId = mVideoStreamId;
                // 视频开始解码
                if (videoDecoder == null) {
                    videoDecoder = new Decoder(decodeType, closeOpengles, closeSmooth, true, surfaceHolder,
                            context);
                } else {
                    videoDecoder.startPlay();
                }
                setCurrentStatus(VmType.PLAY_STATUS_PLAYING);
            } else {
                doCloseStreamTask(playMode, monitorId);
                setCurrentStatus(VmType.PLAY_STATUS_NONE);
            }

            if (mErrorCode == ErrorCode.ERR_CODE_OK && mAudioStreamId > 0) {
                audioStreamId = mAudioStreamId;
                // 音频开始解码
                if (audioDecoder == null) {
                    audioDecoder = new Decoder(decodeType, closeOpengles, closeSmooth, openAudio,
                            surfaceHolder, context);
                } else if (openAudio) {
                    audioDecoder.startPlay();
                }
            }
            startStreamTask = null;
        }

        @Override
        protected void onCancelled(Void aVoid) {
            if (errorCode == ErrorCode.ERR_CODE_OK) {
                Log.e(TAG, "取消取流 mVideoStreamId=" + mVideoStreamId + ", mAudioStreamId=" + mAudioStreamId);
                doStopStreamTask(mVideoStreamId, mAudioStreamId);
            }
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
