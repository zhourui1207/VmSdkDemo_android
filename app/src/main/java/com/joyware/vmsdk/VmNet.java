package com.joyware.vmsdk;

/**
 * Created by zhourui on 16/10/25.
 * sdk通信接口
 */

public class VmNet {
    private static String TAG = "VmNet";
    private static ServerConnectStatusCallback mServerStatusCallback = null;
    private static RealAlarmCallback mRealAlarmCallback = null;

    // 应用启动时，加载VmNet动态库
    static {
        System.loadLibrary("VmNet");
    }

    /**
     * 初始化
     *
     * @param maxThreadCount 最大线程数量
     * @return true 初始化成功 false 初始化失败
     */
    public static boolean init(int maxThreadCount) {
        return Init(maxThreadCount);
    }

    /**
     * 反初始化
     */
    public static void unInit() {
        UnInit();
    }

    /**
     * 连接服务器
     *
     * @param serverAddr                  服务器地址 如"127.0.0.1"
     * @param serverPort                  服务器端口
     * @param serverConnectStatusCallback 服务器连接状态回调
     * @return
     */
    public static int connect(String serverAddr, int serverPort, ServerConnectStatusCallback
            serverConnectStatusCallback) {
        mServerStatusCallback = serverConnectStatusCallback;
        return Connect(serverAddr, serverPort);
    }

    /**
     * 断开服务器
     */
    public static void disconnect() {
        Disconnect();
        mServerStatusCallback = null;
    }

    /**
     * 登录用户
     *
     * @param loginName 用户名
     * @param loginPwd  密码
     * @return
     */
    public static int login(String loginName, String loginPwd) {
        return Login(loginName, loginPwd);
    }

    /**
     * 登出
     */
    public static void logout() {
        Logout();
    }

    /**
     * 获取行政树列表
     *
     * @param pageNo         当前页 现版本无效
     * @param pageSize       页面大小
     * @param depTreesHolder 行政树列表对象
     * @return
     */
    public static int getDepTrees(int pageNo, int pageSize, DepTreesHolder depTreesHolder) {
        return GetDepTrees(pageNo, pageSize, depTreesHolder);
    }

    /**
     * 获取通道列表
     *
     * @param pageNo         当前页 现版本无效
     * @param pageSize       页面大小
     * @param depId          行政树id
     * @param channelsHolder 通道列表对象
     * @return
     */
    public static int getChannels(int pageNo, int pageSize, int depId, ChannelsHolder
            channelsHolder) {
        return GetChannels(pageNo, pageSize, depId, channelsHolder);
    }

    /**
     * 获取录像列表
     *
     * @param pageNo        当前页 现版本无效
     * @param pageSize      页面大小
     * @param fdId          设备id
     * @param channelId     通道号
     * @param beginTime     起始时间
     * @param endTime       截止时间
     * @param isCenter      true：中心录像  false：设备录像
     * @param recordsHolder 录像列表类
     * @return
     */
    public static int getRecords(int pageNo, int pageSize, String fdId, int channelId, int
            beginTime, int endTime, boolean isCenter, RecordsHolder recordsHolder) {
        return GetRecords(pageNo, pageSize, fdId, channelId, beginTime, endTime, isCenter,
                recordsHolder);
    }

    /**
     * 获取报警列表
     *
     * @param pageNo         当前页 现版本无效
     * @param pageSize       页面大小
     * @param fdId           设备id
     * @param channelId      通道号
     * @param channelBigType 通道大类
     * @param beginTime      起始时间
     * @param endTime        截止时间
     * @param alarmsHolder   报警列表类
     * @return
     */
    public static int getAlarms(int pageNo, int pageSize, String fdId, int channelId, int
            channelBigType, String beginTime, String endTime, AlarmsHolder alarmsHolder) {
        return GetAlarms(pageNo, pageSize, fdId, channelId, channelBigType, beginTime, endTime,
                alarmsHolder);
    }

    /**
     * 开始接收实时报警推送
     *
     * @param realAlarmCallback 实时报警回调
     */
    public static void startReceiveRealAlarm(RealAlarmCallback realAlarmCallback) {
        mRealAlarmCallback = realAlarmCallback;
        StartReceiveRealAlarm();
    }

    /**
     * 停止接收实时报警推送
     */
    public static void stopReceiveRealAlarm() {
        mRealAlarmCallback = null;
        StopReceiveRealAlarm();
    }

    /**
     * 打开实时码流
     *
     * @param fdId              设备序列号
     * @param channelId         通道号
     * @param isSub             true：子码流  false：主码流
     * @param playAddressHolder 播放地址对象
     * @return
     */
    public static int openRealplayStream(String fdId, int channelId, boolean isSub,
                                         PlayAddressHolder playAddressHolder) {
        return OpenRealplayStream(fdId, channelId, isSub, playAddressHolder);
    }

    /**
     * 关闭实时码流
     *
     * @param monitorId 监控sessionid，打开码流时可在playAddressHolder中获得
     */
    public static void closeRealplayStream(int monitorId) {
        CloseRealplayStream(monitorId);
    }

    /**
     * 开始对讲
     */
    public static boolean startTalk(StreamCallbackV3 streamCallback) {
        return StartTalk(streamCallback);
    }

    public static boolean sendTalk(String remoteAddress, int remotePort, byte[] data, int
            dataLen) {
        return SendTalk(remoteAddress, remotePort, data, dataLen);
    }

    /**
     * 停止对讲
     */
    public static void stopTalk() {
        StopTalk();
    }

    /**
     * 开始对讲
     */
    public static boolean startStreamHeartbeatServer() {
        return StartStreamHeartbeatServer();
    }

    public static boolean sendHeartbeat(String remoteAddress, int remotePort, int
            heartbeatType, String monitorId, String srcId) {
        return SendHeartbeat(remoteAddress, remotePort, heartbeatType, monitorId, srcId);
    }

    /**
     * 停止对讲
     */
    public static void stopStreamHeartbeatServer() {
        StopStreamHeartbeatServer();
    }

    /**
     * 打开回放码流
     *
     * @param fdId              设备序列号
     * @param channelId         通道号
     * @param isCenter          true：中心录像  false：前端录像
     * @param beginTime         起始时间
     * @param endTime           截止时间
     * @param playAddressHolder 播放地址对象
     * @return
     */
    public static int openPlaybackStream(String fdId, int channelId, boolean isCenter, int
            beginTime, int endTime, PlayAddressHolder playAddressHolder) {
//    Log.e("VmNet", "openPlaybackStream()");
        return OpenPlaybackStream(fdId, channelId, isCenter, beginTime, endTime, playAddressHolder);
    }

    /**
     * 关闭回放码流
     *
     * @param monitorId 监控sessionid，打开码流时可在playAddressHolder中获得
     */
    public static void closePlaybackStream(int monitorId) {
//    Log.e("VmNet", "closePlaybackStream(" + monitorId + ")");
        ClosePlaybackStream(monitorId);
    }

    /**
     * 控制回放
     *
     * @param monitorId 监控sessionid，打开码流时可在playAddressHolder中获得
     * @param controlId 控制id
     * @param action    控制动作
     * @param param     控制参数
     * @return
     */
    public static int controlPlayback(int monitorId, int controlId,
                                      String action, String param) {
        return ControlPlayback(monitorId, controlId, action, param);
    }

    /**
     * 开始获取码流数据
     *
     * @param address        码流获取地址
     * @param port           码流获取端口
     * @param streamCallback 码流回调接口
     * @param streamIdHolder 码流id对象
     * @return
     */
    public static int startStream(String address, int port, StreamCallback streamCallback,
                                  StreamIdHolder streamIdHolder) {
//    Log.e("VmNet", "startStream()");
        return StartStream(address, port, streamCallback, "", "", VmType.PLAY_TYPE_REALPLAY,
                VmType.CLIENT_TYPE_ANDROID, streamIdHolder);
    }

    public static int startStream(String address, int port, StreamCallback streamCallback,
                                  String monitorId, String deviceId, int playType,
                                  int clientType, StreamIdHolder streamIdHolder) {
//    Log.e("VmNet", "startStream()");
        return StartStream(address, port, streamCallback, monitorId, deviceId, playType, clientType,
                streamIdHolder);
    }

    public static int startStream(String address, int port, StreamCallbackExt streamCallback,
                                  String monitorId, String deviceId, int playType,
                                  int clientType, StreamIdHolder streamIdHolder) {
//    Log.e("VmNet", "startStream()");
        return StartStream(address, port, streamCallback, monitorId, deviceId, playType, clientType,
                streamIdHolder);
    }

    /**
     * 停止获取码流数据
     *
     * @param streamId 码流id
     */
    public static void stopStream(int streamId) {
//    Log.e("VmNet", "stopStream(" + streamId + ")");
        StopStream(streamId);
    }

    public static int startStreamByRtsp(String url, boolean encrypt, StreamCallbackV2
            streamCallbackV2) {
        return StartStreamByRtsp(url, encrypt, streamCallbackV2);
    }

    public static void stopStreamByRtsp(int rtspStreamId) {
        StopStreamByRtsp(rtspStreamId);
    }

    public static boolean pauseStreamByRtsp(int rtspStreamid) {
        return PauseStreamByRtsp(rtspStreamid);
    }

    public static boolean playStreamByRtsp(int rtspStreamId) {
        return PlayStreamByRtsp(rtspStreamId);
    }

    public static boolean speedStreamByRtsp(int rtspStreamId, float speed) {
        return SpeedStreamByRtsp(rtspStreamId, speed);
    }

    /**
     * 发送控制指令
     *
     * @param fdId        设备序列号
     * @param channelId   通道号
     * @param controlType 控制类型
     * @param param1      参数1
     * @param param2      参数2
     */
    public static void sendControl(String fdId, int channelId, int controlType, int param1,
                                   int param2) {
        SendControl(fdId, channelId, controlType, param1, param2);
    }

    public static boolean filterRtpHeader(byte[] inBuf, int inStart, int inLen, byte[]
            outBuf, int outStart, int outLen, RtpInfoHolder rtpInfoHolder) {
        return FilterRtpHeader(inBuf, inStart, inLen, outBuf, outStart, outLen, rtpInfoHolder);
    }

    public static boolean filterRtpHeaderExt(byte[] inBuf, int inStart, int inLen, byte[]
            outBuf, int outStart, int outLen, RtpInfoHolder rtpInfoHolder) {
        return FilterRtpHeaderExt(inBuf, inStart, inLen, outBuf, outStart, outLen, rtpInfoHolder);
    }

    // jni层函数---------------------------------------------------------------------------------------
    private static native boolean Init(int maxThreadCount);

    private static native void UnInit();

    private static native int Connect(String serverAddr, int serverPort);

    private static native void Disconnect();

    private static native int Login(String loginName, String loginPwd);

    private static native void Logout();

    private static native int GetDepTrees(int pageNo, int pageSize, DepTreesHolder depTreesHolder);

    private static native int GetChannels(int pageNo, int pageSize, int depId, ChannelsHolder
            channelsHolder);

    private static native int GetRecords(int pageNo, int pageSize, String fdId, int channelId, int
            beginTime, int endTime, boolean isCenter, RecordsHolder recordsHolder);

    private static native int GetAlarms(int pageNo, int pageSize, String fdId, int channelId, int
            channelBigType, String beginTime, String endTime, AlarmsHolder alarmsHolder);

    private static native void StartReceiveRealAlarm();

    private static native void StopReceiveRealAlarm();

    private static native int OpenRealplayStream(String fdId, int channelId, boolean isSub,
                                                 PlayAddressHolder playAddressHolder);

    private static native void CloseRealplayStream(int monitorId);

    private static native int OpenPlaybackStream(String fdId, int channelId, boolean isCenter,
                                                 int beginTime, int
                                                         endTime, PlayAddressHolder
                                                         playAddressHolder);

    private static native void ClosePlaybackStream(int monitorId);

    private static native int ControlPlayback(int monitorId, int controlId,
                                              String action, String param);

    private static native int StartStream(String address, int port, StreamCallback streamCallback,
                                          String monitorId, String deviceId, int playType,
                                          int clientType, StreamIdHolder streamIdHolder);

    private static native int StartStream(String address, int port, StreamCallbackExt
            streamCallback, String monitorId, String deviceId, int playType,
                                             int clientType, StreamIdHolder streamIdHolder);

    private static native void StopStream(int streamId);

    //    private static native int StartStreamByRtsp(String rtspUrl, StreamCallbackV2
    // streamCallback);
    private static native int StartStreamByRtsp(String rtspUrl, boolean encrypt, StreamCallbackV2
            streamCallback);

    private static native void StopStreamByRtsp(int rtspStreamId);

    private static native boolean PauseStreamByRtsp(int rtspStreamId);

    private static native boolean PlayStreamByRtsp(int rtspStreamId);

    private static native boolean SpeedStreamByRtsp(int rtspStreamId, float speed);

    private static native void SendControl(String fdId, int channelId, int controlType, int param1,
                                           int param2);

    private static native boolean StartTalk(StreamCallbackV3 streamCallback);

    private static native boolean SendTalk(String remoteAddress, int remotePort, byte[] data, int
            dataLen);

    private static native void StopTalk();

    private static native boolean StartStreamHeartbeatServer();

    private static native boolean SendHeartbeat(String remoteAddress, int remotePort, int
            heartbeatType, String monitorId, String srcId);

    private static native void StopStreamHeartbeatServer();

    private static native boolean FilterRtpHeader(byte[] inBuf, int inStart, int inLen, byte[]
            outBuf, int outStart, int outLen, RtpInfoHolder rtpInfoHolder);

    private static native boolean FilterRtpHeaderExt(byte[] inBuf, int inStart, int inLen, byte[]
            outBuf, int outStart, int outLen, RtpInfoHolder rtpInfoHolder);

    public static native long JWCipherCreate(byte[] key, int keyLen);

    public static native void JWCipherRelease(long cipherCtx);

    public static native void JWCipherDecryptH264(long cipherCtx, int streamId, byte[] h264data,
                                                  int dataBegin, int dataLen);

    public static native int JWCipherDecrypt(long cipherCtx, byte[] in, int inBegin, int inLen,
                                             byte[] out, int outBegin, int outLen);

    // 回调函数分界线---------------------------------------------------------------------------------------
    private static void onServerConnectStatus(boolean isConnected) {
        if (mServerStatusCallback != null) {
            mServerStatusCallback.onServerConnectStatus(isConnected);
        }
    }

    private static void onStreamConnectStatus(int streamId, boolean isConnected, Object object) {
        if (object != null) {
            if (object instanceof StreamCallback) {
                StreamCallback streamCallback = (StreamCallback) object;
                streamCallback.onStreamConnectStatus(streamId, isConnected);
            } else if (object instanceof StreamCallbackExt) {
                StreamCallbackExt streamCallback = (StreamCallbackExt) object;
                streamCallback.onStreamConnectStatus(streamId, isConnected);
            }
        }
    }

    private static void onStream(int streamId, int streamType, int payloadType, byte[] buffer, int
            len, int timeStamp, int seqNumber, boolean isMark, boolean isJWHeader, boolean
                                         isFirstFrame, boolean isLastFrame, long utcTimeStamp,
                                 Object object) {
        if (object != null) {
            if (object instanceof StreamCallback) {
                StreamCallback streamCallback = (StreamCallback) object;
                streamCallback.onReceiveStream(streamId, streamType, payloadType, buffer,
                        timeStamp, seqNumber, isMark);
            } else if (object instanceof StreamCallbackExt) {
                StreamCallbackExt streamCallback = (StreamCallbackExt) object;
                streamCallback.onReceiveStream(streamId, streamType, payloadType, buffer,
                        timeStamp, seqNumber, isMark, isJWHeader, isFirstFrame, isLastFrame,
                        utcTimeStamp);
            }
        }
    }

    private static void onStreamConnectStatusV2(boolean isConnected, Object object) {
        if (object != null) {
            StreamCallbackV2 streamCallback = (StreamCallbackV2) object;
            streamCallback.onStreamConnectStatus(isConnected);
        }
    }

    private static void onStreamV2(byte[] buffer, int len, Object object) {
        if (object != null) {
            StreamCallbackV2 streamCallback = (StreamCallbackV2) object;
            streamCallback.onReceiveStream(buffer, len);
        }
    }

    private static void onStreamV3(String remoteAddr, int remotePort, byte[] buffer, int len,
                                   Object object) {
        if (object != null) {
            StreamCallbackV3 streamCallback = (StreamCallbackV3) object;
            streamCallback.onReceiveStream(remoteAddr, remotePort, buffer, len);
        }
    }

    private static void onRealAlarm(String fdId, int channelId, int alarmType, int param1, int
            param2) {
        if (mRealAlarmCallback != null) {
            mRealAlarmCallback.onRealAlarm(fdId, channelId, alarmType, param1, param2);
        }
    }

    /**
     * 服务器连接状态回调接口
     */
    public interface ServerConnectStatusCallback {
        void onServerConnectStatus(boolean isConnected);
    }

    /**
     * 码流回调接口
     */
    public interface StreamCallback {
        // 连接状态
        void onStreamConnectStatus(int streamId, boolean isConnected);

        // 接收到码流
        void onReceiveStream(int streamId, int streamType, int payloadType, byte[] buffer,
                             int timeStamp, int seqNumber, boolean isMark);
    }

    public interface StreamCallbackExt {
        // 连接状态
        void onStreamConnectStatus(int streamId, boolean isConnected);

        // 接收到码流
        void onReceiveStream(int streamId, int streamType, int payloadType, byte[] buffer,
                             int timeStamp, int seqNumber, boolean isMark, boolean isJWHeader,
                             boolean isFirstFrame, boolean isLastFrame, long utcTimeStamp);
    }

    /**
     * 码流回调接口
     */
    public interface StreamCallbackV2 {
        // 连接状态
        void onStreamConnectStatus(boolean isConnected);

        // 接收到码流
        void onReceiveStream(byte[] streamData, int streamLen);
    }

    public interface StreamCallbackV3 {
        // 接收到码流
        void onReceiveStream(String remoteAddr, int remotePort, byte[] streamData, int streamLen);
    }

    /**
     * 实时报警回调接口
     */
    public interface RealAlarmCallback {
        void onRealAlarm(String fdId, int channelId, int alarmType, int param1, int param2);
    }

}
