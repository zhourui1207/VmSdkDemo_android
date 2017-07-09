package com.joyware.vmsdk.core;

import android.support.annotation.NonNull;
import android.util.Log;

import java.util.Collections;
import java.util.Iterator;
import java.util.LinkedList;

/**
 * Created by zhourui on 2017/7/7.
 * email: 332867864@qq.com
 * phone: 17006421542
 */

public class RTPSortFilter {
    private static final String TAG = "RTPSortFilter";

    private static final int DEFAULT_MAX_SIZE = 5;

    private static final int MAX_SEQ_NUMBER = 65535;

    @NonNull
    private final LinkedList<RTPData> mSortList = new LinkedList<>();

    private int mMaxSize;

    private int mSeqNumber = -1;  // 实际上是unsigned short，的就是jchar，但是用int比较好计算，范围0-65535

    private OnSortedCallback mOnSortedCallback;

    private OnMissedCallback mOnMissedCallback;

    private boolean mSort;

    public RTPSortFilter() {
        init(DEFAULT_MAX_SIZE);
    }

    public RTPSortFilter(int maxSize) {
        init(maxSize);
    }

    private void init(int maxSize) {
        mMaxSize = maxSize;
    }

    public void setOnSortedCallback(OnSortedCallback onSortedCallback) {
        mOnSortedCallback = onSortedCallback;
    }

    public void setOnMissedCallback(OnMissedCallback onMissedCallback) {
        mOnMissedCallback = onMissedCallback;
    }

    public void setSort(boolean sort) {
        mSort = sort;
    }

    public void receive(int payloadType, byte[] buffer, int start, int len, int timeStamp, int
            seqNumber, boolean mark) {
        if (!mSort && mOnSortedCallback != null) {
            mOnSortedCallback.onSorted(payloadType, buffer, start, len, timeStamp, seqNumber, mark);
            return;
        }

        if (lessThanPredicted(seqNumber)) { // 接收到了一个小包，那么直接丢
            Log.e(TAG, "Receive less than predicted, miss packet! receive seq number:" +
                    seqNumber + ", current seq number:" + mSeqNumber);
            return;
        }

        boolean needSendListSerialPacket = false;  // 需要发送列表中的连续包

        if (isSerial(seqNumber)) {  // 序列连续，直接调用接口
            sendSortedPacket(payloadType, buffer, start, len, timeStamp, seqNumber, mark);
            if (!mSortList.isEmpty()) {  // 如果队列中第一个包的序列号是跟发送出去的包是连续的
                RTPData rtpData = mSortList.getFirst();
                if (isSerial(rtpData.mSeqNumber)) {
                    needSendListSerialPacket = true;
                }
            }
        } else {  // 如果不连续
            if (mSortList.size() >= mMaxSize) {  // 超过或等于最大值，那么需要强制发包
                needSendListSerialPacket = true;
            }
            mSortList.add(new RTPData(payloadType, buffer, start, len, timeStamp, seqNumber, mark));
            Collections.sort(mSortList);
        }

        if (needSendListSerialPacket && !mSortList.isEmpty()) {
            sendSerialFromFirstPacketInList();
        }
    }

    // 小于预计值
    private boolean lessThanPredicted(int seqNumber) {
        return (mSeqNumber >= 0) && (seqNumberCompareTo(seqNumber, mSeqNumber == MAX_SEQ_NUMBER ?
                0 : mSeqNumber + 1) < 0);
    }

    private boolean isSerial(int seqNumber) {
        return (mSeqNumber < 0) || (mSeqNumber == MAX_SEQ_NUMBER && seqNumber == 0) || (seqNumber
                == mSeqNumber + 1);
    }

    private void sendSerialFromFirstPacketInList() {
        // 第一个包是必须发送的，后面的包要判断一下是否跟一个包连续，连续的话就都发送出去，不连续的话就终止
        boolean first = true;
        Iterator<RTPData> it = mSortList.iterator();
        while (it.hasNext()) {
            RTPData rtpData = it.next();
            if (first || isSerial(rtpData.mSeqNumber)) {
                sendSortedPacket(rtpData);
                it.remove();
                first = false;
            } else {
                break;
            }
        }
    }

    private void sendSortedPacket(RTPData rtpData) {
        sendSortedPacket(rtpData.mPayloadType, rtpData.mBuffer, 0, rtpData.mBuffer != null ?
                rtpData.mBuffer.length : 0, rtpData.mTimestamp, rtpData.mSeqNumber, rtpData.mMark);
    }

    private void sendSortedPacket(int payloadType, byte[] buffer, int start, int len, int
            timeStamp, int seqNumber, boolean mark) {
        if (mOnSortedCallback != null) {
            mOnSortedCallback.onSorted(payloadType, buffer, start, len, timeStamp, seqNumber, mark);
        }
        if ((mOnMissedCallback != null) && !isSerial(seqNumber)) {  // 序列包不连续的话，计算丢失包的数量

            int missedStartSeqNumber = mSeqNumber == MAX_SEQ_NUMBER ? 0 : mSeqNumber + 1;
            int missedNumber = seqNumber - missedStartSeqNumber;
            if (missedNumber < 0) {
                missedNumber += MAX_SEQ_NUMBER;
            }
            Log.e(TAG, "missed packet form seq:" + missedStartSeqNumber + ", number:" +
                    missedNumber);
            mOnMissedCallback.onMissed(missedStartSeqNumber, missedNumber);
        }
        mSeqNumber = seqNumber;
    }

    public interface OnSortedCallback {
        void onSorted(int payloadType, byte[] buffer, int start, int len, int timeStamp, int
                seqNumber, boolean mark);
    }

    public interface OnMissedCallback {
        void onMissed(int missedStartSeqNumber, int missedNumber);
    }

    private static class RTPData implements Comparable<RTPData> {
        private int mPayloadType;
        private byte[] mBuffer;
        private int mTimestamp;
        private int mSeqNumber;
        private boolean mMark;

        private RTPData(int payloadType, byte[] buffer, int start, int len, int timestamp,
                        int seqNumber, boolean mark) {
            mPayloadType = payloadType;
            if (len > 0) {
                mBuffer = new byte[len];
                System.arraycopy(buffer, start, mBuffer, 0, len);
            }
            mTimestamp = timestamp;
            mSeqNumber = seqNumber;
            mMark = mark;
        }

        public int getPayloadType() {
            return mPayloadType;
        }

        public byte[] getBuffer() {
            return mBuffer;
        }

        public int getTimestamp() {
            return mTimestamp;
        }

        public int getSeqNumber() {
            return mSeqNumber;
        }

        public boolean isMark() {
            return mMark;
        }

        @Override
        public int compareTo(@NonNull RTPData o) {
            return seqNumberCompareTo(mSeqNumber, o.mSeqNumber);
        }
    }

    private static int seqNumberCompareTo(Integer o1, Integer o2) {
        // 先比较两个序列号相差多少
        int difference = Math.abs(o1 - o2);
        int times = 1;
        if (difference > MAX_SEQ_NUMBER / 2) {  // 序列号相差非常大，那么就认为大的值反而小
            times = -1;
        }
        return o1.compareTo(o2) * times;
    }
}
