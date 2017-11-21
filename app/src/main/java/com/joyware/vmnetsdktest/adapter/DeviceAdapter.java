package com.joyware.vmnetsdktest.adapter;

import android.support.annotation.NonNull;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;

import com.joyware.vmnetsdktest.R;
import com.joyware.vmsdk.JWDeviceInfo;

import java.util.LinkedList;
import java.util.List;

/**
 * Created by zhourui on 2017/11/11.
 * email: 332867864@qq.com
 * phone: 17130045945
 */

public class DeviceAdapter extends BaseAdapter {

    @NonNull
    private final List<JWDeviceInfo> mDevices = new LinkedList<>();

    private final String TAG = "DeviceAdapter";

    public void addDevice(JWDeviceInfo deviceInfo) {
        mDevices.add(deviceInfo);
        Log.w(TAG, "Add device, current size = " + mDevices.size());
        notifyDataSetChanged();
    }

    public void clear() {
        mDevices.clear();
        Log.w(TAG, "Clear device, current size = " + mDevices.size());
        notifyDataSetChanged();
    }

    @Override
    public int getCount() {
        return mDevices.size();
    }

    @Override
    public Object getItem(int position) {
        return mDevices.get(position);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        Log.w(TAG, "Get view, current size = " + mDevices.size() + ", position = " + position +
                ", convertView = " + convertView);
        View view;
        ViewHolder viewHolder;
        if (convertView == null) {
            view = LayoutInflater.from(parent.getContext()).inflate(R.layout.item_device, null);
            viewHolder = new ViewHolder();
            viewHolder.mSerialTv = (TextView) view.findViewById(R.id.tv_serial);
            view.setTag(viewHolder);
        } else {
            view = convertView;
            viewHolder = (ViewHolder) view.getTag();
        }
        JWDeviceInfo deviceInfo = mDevices.get(position);
        if (deviceInfo != null && viewHolder != null) {
            String module = new String(deviceInfo.module).trim();
            String serail = new String(deviceInfo.serial).trim();
            if (viewHolder.mSerialTv != null) {
                viewHolder.mSerialTv.setText("型号:" + module + "，序列号:" + serail);
            }
        }
        return view;
    }

    private static class ViewHolder {
        TextView mSerialTv;
    }
}
