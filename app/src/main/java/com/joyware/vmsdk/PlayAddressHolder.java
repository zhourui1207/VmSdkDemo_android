package com.joyware.vmsdk;

/**
 * Created by zhourui on 16/10/26.
 * 播放地址封装
 */

public class PlayAddressHolder {
    private int monitorId;
    private String videoAddr;
    private int videoPort;
    private String audioAddr;
    private int audioPort;
    private String authKey;
    private String deviceId;
    private int playType;
    private int clientType;

    /**
     * c++层赋值使用
     */
    public void init(int monitorId, String videoAddr, int videoPort, String audioAddr, int
            audioPort) {
        this.monitorId = monitorId;
        this.videoAddr = videoAddr;
        this.videoPort = videoPort;
        this.audioAddr = audioAddr;
        this.audioPort = audioPort;
    }

    public int getMonitorId() {
        return monitorId;
    }

    public void setMonitorId(int monitorId) {
        this.monitorId = monitorId;
    }

    public String getVideoAddr() {
        return videoAddr;
    }

    public void setVideoAddr(String videoAddr) {
        this.videoAddr = videoAddr;
    }

    public int getVideoPort() {
        return videoPort;
    }

    public void setVideoPort(int videoPort) {
        this.videoPort = videoPort;
    }

    public String getAudioAddr() {
        return audioAddr;
    }

    public void setAudioAddr(String audioAddr) {
        this.audioAddr = audioAddr;
    }

    public int getAudioPort() {
        return audioPort;
    }

    public void setAudioPort(int audioPort) {
        this.audioPort = audioPort;
    }

    public String getAuthKey() {
        return authKey;
    }

    public void setAuthKey(String authKey) {
        this.authKey = authKey;
    }

    public String getDeviceId() {
        return deviceId;
    }

    public void setDeviceId(String deviceId) {
        this.deviceId = deviceId;
    }

    public int getPlayType() {
        return playType;
    }

    public void setPlayType(int playType) {
        this.playType = playType;
    }

    public int getClientType() {
        return clientType;
    }

    public void setClientType(int clientType) {
        this.clientType = clientType;
    }
}
