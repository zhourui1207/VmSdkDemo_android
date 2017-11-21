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

    private boolean isJWHeader;

    private boolean isFirstFrame;

    private boolean isLastFrame;

    private long utcTimeStamp;

    public void init(int playloadType, int playloadLen, int seqNumber, int timestamp, boolean
            isMark) {
        this.playloadType = playloadType;
        this.playloadLen = playloadLen;
        this.seqNumber = seqNumber;
        this.timestamp = timestamp;
        this.isMark = isMark;
    }

    public void init(int playloadType, int playloadLen, int seqNumber, int timestamp, boolean
            isMark, boolean isJWHeader, boolean isFirstFrame, boolean isLastFrame, long
            utcTimeStamp) {
        this.playloadType = playloadType;
        this.playloadLen = playloadLen;
        this.seqNumber = seqNumber;
        this.timestamp = timestamp;
        this.isMark = isMark;
        this.isJWHeader = isJWHeader;
        this.isFirstFrame = isFirstFrame;
        this.isLastFrame = isLastFrame;
        this.utcTimeStamp = utcTimeStamp;
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

    public boolean isJWHeader() {
        return isJWHeader;
    }

    public void setJWHeader(boolean JWHeader) {
        isJWHeader = JWHeader;
    }

    public boolean isFirstFrame() {
        return isFirstFrame;
    }

    public void setFirstFrame(boolean firstFrame) {
        isFirstFrame = firstFrame;
    }

    public boolean isLastFrame() {
        return isLastFrame;
    }

    public void setLastFrame(boolean lastFrame) {
        isLastFrame = lastFrame;
    }

    public long getUtcTimeStamp() {
        return utcTimeStamp;
    }

    public void setUtcTimeStamp(long utcTimeStamp) {
        this.utcTimeStamp = utcTimeStamp;
    }
}
