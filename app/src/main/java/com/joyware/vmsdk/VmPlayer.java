package com.joyware.vmsdk;

import android.graphics.Bitmap;
import android.os.AsyncTask;
import android.os.Handler;
import android.os.Looper;
import android.support.annotation.NonNull;
import android.support.annotation.UiThread;
import android.util.Log;
import android.view.SurfaceHolder;

import com.joyware.vmsdk.core.Decoder;
import com.joyware.vmsdk.core.GLRenderSurface;
import com.joyware.vmsdk.core.JWAsyncTask;
import com.joyware.vmsdk.core.RecordThread;
import com.joyware.vmsdk.core.YUVRenderer;
import com.joyware.vmsdk.util.ThreadUtil;

import static com.joyware.vmsdk.VmType.PLAY_STATUS_PLAYING;

/**
 * Created by zhourui on 16/10/28.
 * sdk解码接口，解码接口依赖通信接口
 */

public class VmPlayer implements Decoder.OnESFrameDataCallback {

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

    private boolean openAudio = false;

    private int monitorId;
    private int videoStreamId;
    private int audioStreamId;
    private OpenStreamTask openStreamTask;
    private StartStreamTask startStreamTask;
    private Decoder mDecoder;
    //    private Decoder audioDecoder;
    private String recordFile;

    @NonNull
    private RecordThread mRecordThread = new RecordThread();

    @NonNull
    private YUVRenderer mYUVRenderer = new YUVRenderer();  // yuv渲染器

    private CheckStreamTimeoutTask checkStreamTimeoutTask;

    /**
     * 最后接受到码流的时间
     */
    private long lastReceiveStreamTime;

    /**
     * 自动尝试重连次数
     */
    private int autoTryReconnectCount = 0;

    /**
     * 当前尝试重连的次数
     */
    private int currentTryReconnectCount = 0;

    // 应用启动时，加载VmPlayer动态库
    static {
        System.loadLibrary("JWEncdec");  // 某些设备型号必须要加上依赖的动态库
        System.loadLibrary("VmPlayer");
    }

    /**
     * 设置自动尝试重连次数
     *
     * @param count 0：不重连  <0 永远重连  >0 重连相应地次数，如果尝试次数用完，就是尝试发送超时回调
     */
    public void setAutoTryReconnectCount(int count) {
        autoTryReconnectCount = count;
        doStartCheckStream();
    }

    /**
     * 超时秒数，默认10秒
     */
    private int timeoutSeconds = 15;

    /**
     * 设置超时秒数 >0 才生效
     *
     * @param seconds
     */
    public void setTimeoutSeconds(int seconds) {
        timeoutSeconds = seconds;
    }

    private TimeoutListener timeoutListener;

    @Override
    public void onFrameData(boolean video, int timestamp, long pts, byte[] data, int start, int
            len) {
        if (recordFile != null) {
            mRecordThread.write(video, timestamp, pts, data, start, len);
        }
    }

    /**
     * 码流超时回调
     */
    public interface TimeoutListener {
        // 超时
        void onTimeout();
    }

    public void setTimeoutListener(TimeoutListener timeoutListener) {
        this.timeoutListener = timeoutListener;
    }

    /**
     * 获取是否是中心录像回放
     *
     * @return
     */
    public synchronized boolean isCenter() {
        return isCenter;
    }

    /**
     * 获取监控ID，客户可以在视频播放后，使用此监控id调用网络sdk的其他接口
     *
     * @return
     */
    public synchronized int getMonitorId() {
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
    public synchronized int getPlayMode() {
        return playMode;
    }

    /**
     * 获取当前状态
     *
     * @return
     */
    public synchronized int getCurrentStatus() {
        return currentStatus;
    }

    public void setCurrentStatus(final int status) {
        if (ThreadUtil.isMainThread()) {
            setCurrentStatusOnUIThread(status);
        } else {
            Handler mainHandler = new Handler(Looper.getMainLooper());
            mainHandler.post(new Runnable() {
                @Override
                public void run() {
                    setCurrentStatusOnUIThread(status);
                }
            });
        }
    }

    @UiThread
    private void setCurrentStatusOnUIThread(int status) {
        if (currentStatus == status) {
            return;
        }
        currentStatus = status;
        if (playStatusListener != null) {
            playStatusListener.onStatusChanged(currentStatus);
        }
    }

    public void setSurfaceHolder(SurfaceHolder surfaceHolder) {
        GLRenderSurface glRenderSurface = mYUVRenderer.getRenderSurface();
        if (glRenderSurface != null) {
            // 变化的时候
            if (glRenderSurface.getSurfaceHolder() != surfaceHolder) {
                // surfaceHolder改变，必须停止glrender
                mYUVRenderer.setRenderSurface(null);
            }
        }

        if (glRenderSurface == null && surfaceHolder != null) {  // 不为空，则创建渲染表面
            glRenderSurface = new GLRenderSurface(surfaceHolder);
            mYUVRenderer.setRenderSurface(glRenderSurface);
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
     * @param fdId       设备id
     * @param channelId  通道号
     * @param isSub      是否子码流
     * @param decodeType 解码类型
     * @param openAudio  是否打开音频
     * @return true：开始执行播放任务；false：未执行播放任务（通常是正在播放录像原因）
     */
    public synchronized boolean startRealplay(String fdId, int channelId, boolean isSub, int
            decodeType, boolean openAudio) {
//    if (playMode == VmType.PLAY_MODE_PLAYBACK) {
//      return false;
//    }

        // 如果之前已经在播放，那么需要先停止
        if (currentStatus != VmType.PLAY_STATUS_NONE) {
            stopPlay();
        }

        playMode = VmType.PLAY_MODE_REALPLAY;
        this.fdId = fdId;
        this.channelId = channelId;
        this.isSub = isSub;
        this.decodeType = decodeType;
        this.openAudio = openAudio;

        doOpenStreamTask();
        doStartCheckStream();

        return true;
    }

    public void setPlayMode(int playMode) {
        this.playMode = playMode;
    }

    /**
     * 播放实时流，此函数为异步接口
     *
     * @param videoAddr  视频地址
     * @param videoPort  视频端口
     * @param audioAddr  音频地址
     * @param audioPort  音频端口
     * @param decodeType 解码类型
     * @param openAudio  是否打开音频
     * @return true：开始执行播放任务；false：未执行播放任务（通常是正在播放录像原因）
     */
    public synchronized boolean startRealplay(String videoAddr, int videoPort, String audioAddr,
                                              int audioPort, int decodeType,
                                              boolean openAudio) {
//    if (playMode == VmType.PLAY_MODE_PLAYBACK) {
//      return false;
//    }

        // 如果之前已经在播放，那么需要先停止
        if (currentStatus != VmType.PLAY_STATUS_NONE) {
            stopPlay();
        }
//        setCurrentStatus(VmType.PLAY_STATUS_BUSY);

        playMode = VmType.PLAY_MODE_REALPLAY;
        this.decodeType = decodeType;
        this.openAudio = openAudio;

        PlayAddressHolder holder = new PlayAddressHolder();
        holder.init(0, videoAddr, videoPort, audioAddr, audioPort);
        doStartStreamTask(holder);

        doStartCheckStream();

        return true;
    }

    /**
     * 录像回放，此函数为异步接口
     *
     * @param fdId       设备id
     * @param channelId  通道号
     * @param isCenter   是否中心录像
     * @param beginTime  录像起始时间
     * @param endTime    录像截止时间
     * @param decodeType 解码类型
     * @param openAudio  是否打开音频
     * @return true：开始执行播放任务；false：未执行播放任务（通常是正在播放实时预览原因）
     */
    public synchronized boolean startPlayback(String fdId, int channelId, boolean isCenter, int
            beginTime, int endTime, int decodeType, boolean openAudio) {
//    if (playMode == VmType.PLAY_MODE_REALPLAY) {
//      return false;
//    }

        // 如果之前已经在播放，那么需要先停止
        if (currentStatus != VmType.PLAY_STATUS_NONE) {
            stopPlay();
        }

        playMode = VmType.PLAY_MODE_PLAYBACK;
        this.fdId = fdId;
        this.channelId = channelId;
        this.isCenter = isCenter;
        this.beginTime = beginTime;
        this.endTime = endTime;
        this.decodeType = decodeType;
        this.openAudio = openAudio;

        doOpenStreamTask();

        doStartCheckStream();

        return true;
    }

    /**
     * 录像回放，此函数为异步接口
     *
     * @param videoAddr  视频地址
     * @param videoPort  视频端口
     * @param audioAddr  音频地址
     * @param audioPort  音频端口
     * @param decodeType 解码类型
     * @param openAudio  是否打开音频
     * @return true：开始执行播放任务；false：未执行播放任务（通常是正在播放实时预览原因）
     */
    public synchronized boolean startPlayback(String videoAddr, int videoPort, String audioAddr,
                                              int audioPort, int decodeType,
                                              boolean openAudio) {
//    if (playMode == VmType.PLAY_MODE_REALPLAY) {
//      return false;
//    }

        // 如果之前已经在播放，那么需要先停止
        if (currentStatus != VmType.PLAY_STATUS_NONE) {
            stopPlay();
        }
//        setCurrentStatus(VmType.PLAY_STATUS_BUSY);

        playMode = VmType.PLAY_MODE_PLAYBACK;
        this.decodeType = decodeType;
        this.openAudio = openAudio;

        PlayAddressHolder holder = new PlayAddressHolder();
        holder.init(0, videoAddr, videoPort, audioAddr, audioPort);
        doStartStreamTask(holder);

        doStartCheckStream();

        return true;
    }

    public synchronized boolean controlPlayback(String action, String param) {
        if (playMode != VmType.PLAY_MODE_PLAYBACK || currentStatus != PLAY_STATUS_PLAYING) {
            return false;
        }

        int ret = VmNet.controlPlayback(monitorId, 0, action, param);
        if (ret == ErrorCode.ERR_CODE_OK) {
            return true;
        }
        return false;
    }

    public void scaleTo(boolean enable, float leftPercent, float topPercent, float
            widthPercent, float heightPercent) {
        mYUVRenderer.scaleChanged(enable, leftPercent, topPercent, widthPercent, heightPercent);
    }

    /**
     * 停止播放
     */
    public synchronized void stopPlay() {
        doStopCheckStream();

//        playStatusListener = null;
//        streamStatusListener = null;
        setCurrentStatus(VmType.PLAY_STATUS_NONE);
        // 下面皆为同步操作
        doCloseStreamTask(playMode, monitorId);
        doStopStreamTask(videoStreamId, audioStreamId);
        videoStreamId = 0;
        audioStreamId = 0;
        doStopDecodeTask();
//        playMode = VmType.PLAY_MODE_NONE;
    }

    /**
     * 暂停显示
     */
    public void pauseDisplay() {
        doStopCheckStream();
        if (mDecoder != null) {
            mDecoder.pauseDisplay();
        }
    }

    /**
     * 继续显示
     */
    public void continueDisplay() {
        doStartCheckStream();
        if (mDecoder != null) {
            mDecoder.continueDisplay();
        }
    }

    public boolean isOpenAudio() {
        return openAudio;
    }

    /**
     * 打开声音
     *
     * @return
     */
    public boolean openAudio() {
        if (mDecoder != null) {
            mDecoder.openAudio();
        }
//        if (audioDecoder != null) {
//            audioDecoder.startPlay();
//            audioDecoder.openAudio();
//        }
        openAudio = true;
        return openAudio;
    }

    /**
     * 关闭声音
     */
    public void closeAudio() {
        if (mDecoder != null) {
            mDecoder.closeAudio();
        }
//        if (audioDecoder != null) {
//            audioDecoder.closeAudio();
//            audioDecoder.stopPlay();
//        }
        openAudio = false;
    }

    public boolean isRecording() {
//        return mDecoder != null && mDecoder.isRecording();
        return mDecoder != null && mRecordThread.recording();
    }

    public String getRecordFile() {
        return recordFile;
    }

    /**
     * 开始实时录像，若目录不存在，需要客户自己去创建目录
     *
     * @param file 生成文件的全路径 如 /sdcard/MVSS/LocalRecord/record2014-12-07.flv
     * @return
     */
    public boolean startRecord(String file) {
//        if (mDecoder != null) {
//            if (mDecoder.startRecord(file)) {
//                recordFile = file;
//                return true;
//            } else {
//                return false;
//            }
//        }
//        return false;
        if (mDecoder != null) {
            if (mRecordThread.startRecord(RecordThread.RECORD_FILE_TYPE_MP4, file)) {
                recordFile = file;
                return true;
            } else {
                return false;
            }
        }
        return false;
    }

    /**
     * 停止实时录像
     */
    public boolean stopRecord() {
//        if (mDecoder != null) {
//            mDecoder.stopRecord();
//        }
//        recordFile = null;
        recordFile = null;
        return mDecoder != null && mRecordThread.stopRecord(true);
    }

    /**
     * 截图，保存成jpeg/jpg格式
     *
     * @param file 截图后生成文件路径
     * @return
     */
    public Bitmap screenshot(String file, ScreenshotCallback callback) {
        if (mDecoder != null) {
            return mDecoder.screenshot(file, callback);
        }
        return null;
    }

    public boolean capture(String file, CaptureCallback callback) {
        if (mDecoder != null) {
            return mYUVRenderer.capture(file, callback);
        }
        return false;
    }

    private void doOpenStreamTask() {
//        setCurrentStatus(VmType.PLAY_STATUS_BUSY);
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
            VmNet.stopStream(videoStreamId);
        }
        if (audioStreamId != 0) {
            VmNet.stopStream(audioStreamId);
        }
    }

    private void doStopDecodeTask() {
        if (mDecoder != null) {
            mDecoder.stopPlay();
            mDecoder = null;
        }
        mRecordThread.stopRecord(true);

//        if (audioDecoder != null) {
//            audioDecoder.stopPlay();
//            audioDecoder = null;
//        }
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
        protected void onPreExecute() {
            setCurrentStatus(VmType.PLAY_STATUS_OPENSTREAMING);
        }

        @Override
        protected PlayAddressHolder doInBackground(Void... voids) {
            PlayAddressHolder holder = new PlayAddressHolder();
            mPlayMode = playMode;
            switch (playMode) {
                case VmType.PLAY_MODE_REALPLAY:
                    mErrorCode = VmNet.openRealplayStream(fdId, channelId, isSub, holder);
                    break;
                case VmType.PLAY_MODE_PLAYBACK:
                    mErrorCode = VmNet.openPlaybackStream(fdId, channelId, isCenter, beginTime,
                            endTime,
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
                Log.e(TAG, "取消打开码流 mPlatMode=" + mPlayMode + ", monitorId=" + holder.getMonitorId
                        ());
                doCloseStreamTask(mPlayMode, holder.getMonitorId());
            }
        }
    }

    private class StartStreamTask extends AsyncTask<PlayAddressHolder, Void, Void> {

        int mErrorCode = ErrorCode.NO_EXECUTE;

        int mVideoStreamId;
        int mAudioStreamId;

        @Override
        protected void onPreExecute() {
            setCurrentStatus(VmType.PLAY_STATUS_CONNECTING);
        }

        @Override
        protected Void doInBackground(PlayAddressHolder... playAddressHolders) {
            PlayAddressHolder holder = playAddressHolders[0];

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
            if (holder.getAudioAddr() != null && holder.getAudioAddr().length() > 0 && holder
                    .getAudioPort() >= 0) {
                StreamIdHolder audioStreamIdHolder = new StreamIdHolder();
                // 若音频取流失败，则不做任何处理 todo:现在的音频类型是有问题的，因为音频的头居然使用的是视频的头
                int ret = VmNet.startStream(holder.getAudioAddr(), holder.getAudioPort(), new
                        AudioStreamCallbackI(), audioStreamIdHolder);

                if (ret == ErrorCode.ERR_CODE_OK) {
                    mAudioStreamId = audioStreamIdHolder.getStreamId();
                }
            }

            return null;
        }

        @Override
        protected void onPostExecute(Void aVoid) {
            errorCode = mErrorCode;
            boolean closeSmooth = playMode == VmType.PLAY_MODE_PLAYBACK;

            if (mErrorCode == ErrorCode.ERR_CODE_OK) {
                setCurrentStatus(VmType.PLAY_STATUS_WAITSTREAMING);
                videoStreamId = mVideoStreamId;
                // 视频开始解码
                if (mDecoder == null) {
                    mDecoder = new Decoder(decodeType, true, openAudio, playMode == VmType
                            .PLAY_MODE_REALPLAY);
                    mDecoder.setOnESFrameDataCallback(VmPlayer.this);
                    mDecoder.setSeepScale(mSpeedScale);
                    mDecoder.setOnYUVFrameDataCallback(new Decoder.OnYUVFrameDataCallback() {
                        @Override
                        public void onFrameData(int width, int height, byte[] yData, int yStart,
                                                int yLen, byte[] uData, int uStart, int uLen,
                                                byte[] vData, int vStart, int vLen) {
                            mYUVRenderer.frameChanged(width, height, yData, yStart, yLen, uData,
                                    uStart, uLen, vData, vStart, vLen);
                        }
                    });
                } else {
                    mDecoder.startPlay();
                }
            } else {
                doCloseStreamTask(playMode, monitorId);
                setCurrentStatus(VmType.PLAY_STATUS_NONE);
            }

            if (mErrorCode == ErrorCode.ERR_CODE_OK && mAudioStreamId > 0) {
                audioStreamId = mAudioStreamId;
                // 音频开始解码
//                if (audioDecoder == null) {
//                    audioDecoder = new Decoder(decodeType, openAudio,
//                            openAudio, true);
//                } else if (openAudio) {
//                    audioDecoder.startPlay();
//                }
            }
            startStreamTask = null;
        }

        @Override
        protected void onCancelled(Void aVoid) {
            if (errorCode == ErrorCode.ERR_CODE_OK) {
                Log.e(TAG, "取消取流 mVideoStreamId=" + mVideoStreamId + ", mAudioStreamId=" +
                        mAudioStreamId);
                doStopStreamTask(mVideoStreamId, mAudioStreamId);
            }
        }
    }

    private float mSpeedScale = 1f;

    public void setSpeedScale(float speedScale) {
        if (mDecoder != null) {
            mDecoder.setSeepScale(speedScale);
        }
        mSpeedScale = speedScale;
    }

    public float getSpeedScale(float speedScale) {
        return mSpeedScale;
    }

    private class VideoStreamCallbackI implements VmNet.StreamCallback {
        private boolean isFirst = true;
        private long receiveCount = 0;
        private int seqNumber = 0;

        @Override
        public void onStreamConnectStatus(int streamId, boolean isConnected) {
            if (streamStatusListener != null) {
                streamStatusListener.onVideoStreamStatus(isConnected);
            }
        }

        @Override
        public void onReceiveStream(int streamId, int streamType, int payloadType, byte[] buffer,
                                    int timeStamp, int seqNumber, boolean isMark) {
            lastReceiveStreamTime = System.currentTimeMillis();
            currentTryReconnectCount = 0;

            if (this.seqNumber == 0) {
                this.seqNumber = seqNumber;
            } else {
                if (this.seqNumber + 1 != seqNumber) {
                    Log.e(TAG, "miss packet " + this.seqNumber + " ==> " + seqNumber);
                }
                this.seqNumber = seqNumber;
            }

//            ++receiveCount;
//            if (receiveCount % 50 == 0) { // 2%丢包率
//                Log.e(TAG, "模拟丢包环境");
//                return;
//            }

//      Log.e(TAG, "!!");
//            Log.e(TAG, "len=" + buffer.length + ", seqNumber=" + seqNumber +", isMark=" + isMark + ", playloadType=" + payloadType);
            if (mDecoder != null) {
                mDecoder.addBuffer(streamId, streamType, payloadType, buffer, 0, buffer.length, timeStamp,
                        seqNumber, isMark);
            }

            if (isFirst && currentStatus == VmType.PLAY_STATUS_WAITSTREAMING) {  // I帧,sps,或pps
                // 这里需要使用主线程回调状态为PLAYING
                setCurrentStatus(VmType.PLAY_STATUS_PLAYING);
                isFirst = false;
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
        public void onReceiveStream(int streamId, int streamType, int payloadType, byte[] buffer,
                                    int timeStamp, int seqNumber, boolean isMark) {

//            if (audioDecoder != null) {
//                audioDecoder.addBuffer(new StreamData(streamId, VmType.STREAM_TYPE_AUDIO,
//                        payloadType, buffer, 0, buffer.length, timeStamp, seqNumber, isMark));
//            }
            if (mDecoder != null) {
                mDecoder.addBuffer(streamId, VmType.STREAM_TYPE_AUDIO, payloadType, buffer, 0, buffer.length, timeStamp,
                        seqNumber, isMark);
            }
        }
    }

    public void inputData(int streamType, byte[] buffer, int bufferStart, int bufferLen, int
            playloadType, int timeStamp, int seqNumber, boolean mark) {
        lastReceiveStreamTime = System.currentTimeMillis();
        currentTryReconnectCount = 0;

        // 视频开始解码
        if (mDecoder == null) {
            mDecoder = new Decoder(decodeType, true, openAudio, playMode == VmType
                    .PLAY_MODE_REALPLAY);
            mDecoder.setOnESFrameDataCallback(VmPlayer.this);
            mDecoder.setSeepScale(mSpeedScale);
            mDecoder.setOnYUVFrameDataCallback(new Decoder.OnYUVFrameDataCallback() {
                @Override
                public void onFrameData(int width, int height, byte[] yData, int yStart,
                                        int yLen, byte[] uData, int uStart, int uLen,
                                        byte[] vData, int vStart, int vLen) {
                    mYUVRenderer.frameChanged(width, height, yData, yStart, yLen, uData,
                            uStart, uLen, vData, vStart, vLen);
                }
            });
        } else {
            if (!mDecoder.isPlaying()) {
                mDecoder.startPlay();
            }
        }
        if (mDecoder != null) {
            mDecoder.addBuffer(0, streamType, playloadType, buffer, bufferStart, bufferLen, timeStamp, seqNumber, mark);
            if (currentStatus != VmType.PLAY_STATUS_PLAYING) {
                setCurrentStatus(VmType.PLAY_STATUS_PLAYING);
            }
        }
    }

    public void doStartCheckStream() {
        doStopCheckStream();
        currentTryReconnectCount = 0;
        checkStream();
    }

    private void checkStream() {
        checkStreamTimeoutTask = new CheckStreamTimeoutTask();
        checkStreamTimeoutTask.execute();
//        checkStreamTimeoutTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    public void doStopCheckStream() {
        lastReceiveStreamTime = 0;
        if (checkStreamTimeoutTask != null) {
            checkStreamTimeoutTask.cancel(true);
        }
    }

    private void TimeoutCallback() {
        if (timeoutListener != null) {
            Log.w(TAG, "回调码流超时接口");
            timeoutListener.onTimeout();
        }
    }

    /**
     * 检查码流超时任务，因为考虑到符合流，所以暂时只考虑检测视频流
     */
    private class CheckStreamTimeoutTask extends JWAsyncTask<Void, Void, Boolean> {

        @Override
        protected Boolean doInBackground(Void... params) throws InterruptedException {
            /**
             * 如果正在播放中状态，并且最后收到码流的时间距离现在大于15秒时被认定为超时
             */
            // 延时一秒检测
            Thread.sleep(1000);

//                Log.e(TAG, "检测码流");

            if (lastReceiveStreamTime <= 0) {  // 如果最后接收时间小于等于0，表示刚开始计算时间
                lastReceiveStreamTime = System.currentTimeMillis();
            } else if ((lastReceiveStreamTime > 0) && ((System.currentTimeMillis() -
                    lastReceiveStreamTime) > (timeoutSeconds * 1000))) {
                Log.e(TAG, "检测码流 超时!");
                return true;
            }

//            Log.e(TAG, "检测码流未超时 lastReceiveStreamTime=" + lastReceiveStreamTime);
            return false;
        }

        @Override
        protected void onPostExecute(Boolean timout) {
            if (timout) {  // 超时
                // 判断是否重连
                if (autoTryReconnectCount < 0 || // 小于0表示永远重连
                        (autoTryReconnectCount > 0 && currentTryReconnectCount <
                                autoTryReconnectCount)) {  // 需要重连
                    // 重连次数加1
                    ++currentTryReconnectCount;
                    // 重连
                    reconnect();
                } else {  // 不需要重连的话，就回调给上层超时消息
                    stopPlay();  // 先停止播放吧，这时候肯定是播放不了了
                    TimeoutCallback();
                }
            } else {  // 没超时的话继续检查码流
                checkStream();
            }
        }
    }

    /**
     * 重连
     */
    private void reconnect() {
        Log.w(TAG, "开始重连... 当前次数=" + currentTryReconnectCount);
        stopPlay();

        // 如果是通过fdid取流，需要从打开码流开始，如果是从videoAddress方式播放的话，貌似就没用了
        doOpenStreamTask();
        // 继续检查码流
        checkStream();
    }

    public void release() {
        mYUVRenderer.setRenderSurface(null);
//        if (audioDecoder != null) {
//            audioDecoder.stopPlay();
//            audioDecoder = null;
//        }
        if (checkStreamTimeoutTask != null) {
            checkStreamTimeoutTask.cancel(true);
            checkStreamTimeoutTask = null;
        }
        if (openStreamTask != null) {
            openStreamTask.cancel(true);
            openStreamTask = null;
        }
        if (startStreamTask != null) {
            startStreamTask.cancel(true);
            startStreamTask = null;
        }
        if (mDecoder != null) {
            mDecoder.stopPlay();
            mDecoder = null;
        }
        mRecordThread.stopRecord(true);
    }
}
