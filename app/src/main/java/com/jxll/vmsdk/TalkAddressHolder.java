package com.jxll.vmsdk;

/**
 * Created by zhourui on 16/10/26.
 * 播放地址封装
 */

public class TalkAddressHolder {
    private int talkId;
    private String talkAddr = "";
    private int talkPort;

    /**
     * c++层赋值使用
     */
    public void init(int talkId, String talkAddr, int talkPort) {
        this.talkId = talkId;
        this.talkAddr = talkAddr;
        this.talkPort = talkPort;
    }

    public int getTalkId() {
        return talkId;
    }

    public void setTalkId(int talkId) {
        this.talkId = talkId;
    }

    public String getTalkAddr() {
        return talkAddr;
    }

    public void setTalkAddr(String talkAddr) {
        this.talkAddr = talkAddr;
    }

    public int getTalkPort() {
        return talkPort;
    }

    public void setTalkPort(int talkPort) {
        this.talkPort = talkPort;
    }
}
