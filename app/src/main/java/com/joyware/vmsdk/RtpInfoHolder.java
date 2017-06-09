package com.joyware.vmsdk;

/**
 * Created by zhourui on 16/10/26.
 */

public class RtpInfoHolder {
    private int playloadType;

    private int playloadLen;

    private int seqNumber;

    private int timestamp;

    private boolean isMark;

    public void init(int playloadType, int playloadLen, int seqNumber, int timestamp, boolean isMark) {
        this.playloadType = playloadType;
        this.playloadLen = playloadLen;
        this.seqNumber = seqNumber;
        this.timestamp = timestamp;
        this.isMark = isMark;
    }

    public int getPlayloadType() {
        return playloadType;
    }

    public void setPlayloadType(int playloadType) {
        this.playloadType = playloadType;
    }

    public int getPlayloadLen() {
        return playloadLen;
    }

    public void setPlayloadLen(int playloadLen) {
        this.playloadLen = playloadLen;
    }

    public int getSeqNumber() {
        return seqNumber;
    }

    public void setSeqNumber(int seqNumber) {
        this.seqNumber = seqNumber;
    }

    public int getTimestamp() {
        return timestamp;
    }

    public void setTimestamp(int timestamp) {
        this.timestamp = timestamp;
    }

    public boolean isMark() {
        return isMark;
    }

    public void setMark(boolean mark) {
        isMark = mark;
    }
}
