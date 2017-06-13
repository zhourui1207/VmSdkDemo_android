package com.joyware.vmsdk.core;

import android.support.annotation.NonNull;

/**
 * Created by zhourui on 17/6/13.
 * email: 332867864@qq.com
 * phone: 17006421542
 */

public class PriorityData implements Comparable<PriorityData> {
    private String mMessage;
    private int mSeqNumber;
    private int mRecNumber;


    public PriorityData(String message, int seqNumber, int recNumber) {
        mMessage = message;
        mSeqNumber = seqNumber;
        mRecNumber = recNumber;
    }

    public String getMessage() {
        return mMessage;
    }

    public int getSeqNumber() {
        return mSeqNumber;
    }

    @Override
    public int compareTo(@NonNull PriorityData o) {
        int compare = ((Integer) mSeqNumber).compareTo(o.mSeqNumber);
        if (compare == 0) {
            compare = ((Integer) mRecNumber).compareTo(o.mRecNumber);
        }
        return compare;
    }

    @Override
    public String toString() {
        return "PriorityData{" +
                "mMessage='" + mMessage + '\'' +
                ", mSeqNumber=" + mSeqNumber +
                '}';
    }
}
