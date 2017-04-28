package com.joyware.vmsdk;

/**
 * Created by zhourui on 16/10/27.
 * 视频通道
 */

public class Channel {
    private int depId;
    private String fdId;
    private int channelId;
    private int channelType;
    private String channelName;
    private int isOnline;
    private int videoState;
    private int channelState;
    private int recordState;

    public Channel(int depId, String fdId, int channelId, int channelType, String channelName, int
            isOnline, int videoState, int channelState, int recordState) {
        this.depId = depId;
        this.fdId = fdId;
        this.channelId = channelId;
        this.channelType = channelType;
        this.channelName = channelName;
        this.isOnline = isOnline;
        this.videoState = videoState;
        this.channelState = channelState;
        this.recordState = recordState;
    }

    public int getDepId() {
        return depId;
    }

    public void setDepId(int depId) {
        this.depId = depId;
    }

    public String getFdId() {
        return fdId;
    }

    public void setFdId(String fdId) {
        this.fdId = fdId;
    }

    public int getChannelId() {
        return channelId;
    }

    public void setChannelId(int channelId) {
        this.channelId = channelId;
    }

    public int getChannelType() {
        return channelType;
    }

    public void setChannelType(int channelType) {
        this.channelType = channelType;
    }

    public String getChannelName() {
        return channelName;
    }

    public void setChannelName(String channelName) {
        this.channelName = channelName;
    }

    public int getIsOnline() {
        return isOnline;
    }

    public void setIsOnline(int isOnline) {
        this.isOnline = isOnline;
    }

    public int getVideoState() {
        return videoState;
    }

    public void setVideoState(int videoState) {
        this.videoState = videoState;
    }

    public int getChannelState() {
        return channelState;
    }

    public void setChannelState(int channelState) {
        this.channelState = channelState;
    }

    public int getRecordState() {
        return recordState;
    }

    public void setRecordState(int recordState) {
        this.recordState = recordState;
    }

    @Override
    public String toString() {
        return "Channel{" +
                "depId=" + depId +
                ", fdId='" + fdId + '\'' +
                ", channelId=" + channelId +
                ", channelType=" + channelType +
                ", channelName='" + channelName + '\'' +
                ", isOnline=" + isOnline +
                ", videoState=" + videoState +
                ", channelState=" + channelState +
                ", recordState=" + recordState +
                '}';
    }
}
