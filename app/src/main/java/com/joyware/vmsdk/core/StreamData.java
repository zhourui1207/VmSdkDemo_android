package com.joyware.vmsdk.core;

import android.support.annotation.NonNull;

/**
 * Created by zhourui on 16/10/29.
 * 码流数据对象
 */

public class StreamData implements Comparable<StreamData> {
    private int streamId;
    private int streamType;
    private int payloadType;
    private byte[] buffer;
    private int timeStamp;
    private int seqNumber;
    private boolean isMark;
    private long recNumber;

    private StreamData() {

    }

    public StreamData(int streamId, int streamType, int payloadType, byte[] buffer, int
            bufferStart, int bufferLen, int timeStamp, int seqNumber, boolean isMark, long recNumber) {
        this.streamId = streamId;
        this.streamType = streamType;
        this.payloadType = payloadType;
        // 拷贝数据
        if (buffer != null) {
            this.buffer = new byte[bufferLen];
            System.arraycopy(buffer, bufferStart, this.buffer, 0, bufferLen);
        }
        this.timeStamp = timeStamp;
        this.seqNumber = seqNumber;
        this.isMark = isMark;
        this.recNumber = recNumber;
    }

    public int getStreamId() {
        return streamId;
    }

    public void setStreamId(int streamId) {
        this.streamId = streamId;
    }

    public int getStreamType() {
        return streamType;
    }

    public void setStreamType(int streamType) {
        this.streamType = streamType;
    }

    public int getPayloadType() {
        return payloadType;
    }

    public void setPayloadType(int payloadType) {
        this.payloadType = payloadType;
    }

    public byte[] getBuffer() {
        return buffer;
    }

    public void setBuffer(byte[] buffer) {
        this.buffer = buffer;
    }

    public int getTimeStamp() {
        return timeStamp;
    }

    public void setTimeStamp(int timeStamp) {
        this.timeStamp = timeStamp;
    }

    public int getSeqNumber() {
        return seqNumber;
    }

    public void setSeqNumber(int seqNumber) {
        this.seqNumber = seqNumber;
    }

    public boolean isMark() {
        return isMark;
    }

    public void setMark(boolean mark) {
        isMark = mark;
    }

    @Override
    public int compareTo(@NonNull StreamData o) {
        int compare = ((Integer)seqNumber).compareTo(o.seqNumber);
        if (compare == 0) {
            compare = ((Long)recNumber).compareTo(recNumber);
        }
        return compare;
    }
}
