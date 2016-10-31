package com.jxlianlian.masdk.core;

/**
 * Created by zhourui on 16/10/29.
 * 码流数据对象
 */

public class StreamData {
  private int streamId;
  private int streamType;
  private int payloadType;
  private byte[] buffer;
  private int timeStamp;
  private int seqNumber;
  private boolean isMark;

  private StreamData() {

  }

  public StreamData(int streamId, int streamType, int payloadType, byte[] buffer, int timeStamp,
                    int seqNumber, boolean isMark) {
    this.streamId = streamId;
    this.streamType = streamType;
    this.payloadType = payloadType;
    // 拷贝数据
    if (buffer != null) {
      this.buffer = new byte[buffer.length];
      System.arraycopy(buffer, 0, this.buffer, 0, this.buffer.length);
    }
    this.timeStamp = timeStamp;
    this.seqNumber = seqNumber;
    this.isMark = isMark;
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
}
