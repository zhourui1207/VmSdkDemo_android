package com.joyware.vmnetsdktest;

import android.content.Context;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.joyware.vmnetsdktest.adapter.DeviceAdapter;
import com.joyware.vmsdk.DeviceDiscovery;
import com.joyware.vmsdk.DeviceFindCallBack;
import com.joyware.vmsdk.JWDeviceInfo;
import com.joyware.vmsdk.WifiConfig;

/**
 * Created by zhourui on 2017/11/9.
 * email: 332867864@qq.com
 * phone: 17130045945
 */

public class SmartConfigActivity extends AppCompatActivity {

    private boolean mSending;
    private boolean mDiscoverying;
    private TextView mWifiSsidTv;
    private EditText mWifiPwdTv;
    private Button mButton;
    private ProgressBar mProgressBar;
    private ProgressBar mDiscoveryPb;
    private Button mDiscoveryBtn;
    private ListView mDeviceLv;
    private WifiManager.MulticastLock mMulticastLock;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        allowMulticast();

        initView();
    }

    private void initView() {
        setContentView(R.layout.activity_smart_config);

        mDeviceLv = (ListView) findViewById(R.id.lv_device);
        mDeviceLv.setAdapter(new DeviceAdapter());
        mWifiSsidTv = (TextView) findViewById(R.id.tv_wifi_ssid);
        mWifiSsidTv.setText(getWifiSSID(this));
        mWifiPwdTv = (EditText) findViewById(R.id.et_wifi_pwd);
        mProgressBar = (ProgressBar) findViewById(R.id.pb);
        mButton = (Button) findViewById(R.id.btn_smart_config);
        mButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mSending) {
                    WifiConfig.stop();
                    mSending = false;
                } else {
                    mSending = WifiConfig.config(mWifiSsidTv.getText().toString(), mWifiPwdTv
                            .getText().toString());
                }
                refreshBtn();
            }
        });
        refreshBtn();

        mDiscoveryPb = (ProgressBar) findViewById(R.id.pb_discovery);
        mDiscoveryBtn = (Button) findViewById(R.id.btn_device_discovery);
        mDiscoveryBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mDiscoverying) {
                    DeviceDiscovery.stop();
                    mDiscoverying = false;
                } else {
                    mDiscoverying = DeviceDiscovery.start(new DeviceFindCallBack() {
                        @Override
                        public void onDeviceFindCallBack(final JWDeviceInfo deviceInfo) {
                            String module = new String(deviceInfo.module).trim();
                            String serial = new String(deviceInfo.serial).trim();
                            Log.e("zhouruitest", "module=" + module + ", serial=" + serial);
                            if (mDeviceLv != null) {
                                mDeviceLv.post(new Runnable() {
                                    @Override
                                    public void run() {
                                        DeviceAdapter adapter = (DeviceAdapter) mDeviceLv.getAdapter();
                                        if (adapter != null) {
                                            adapter.addDevice(deviceInfo);
                                        }
                                    }
                                });
                            }
                        }
                    });
                }
                refreshDiscoveryBtn();
            }
        });
        refreshDiscoveryBtn();

        findViewById(R.id.btn_clearup).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                DeviceDiscovery.clearup();
                if (mDeviceLv != null) {
                    DeviceAdapter adapter = (DeviceAdapter) mDeviceLv.getAdapter();
                    if (adapter != null) {
                        adapter.clear();
                    }
                }
            }
        });
    }

    private static String getWifiSSID(Context ctx) {
        WifiManager wifi_service = (WifiManager) ctx.getSystemService(Context.WIFI_SERVICE);
        WifiInfo connectionInfo = wifi_service.getConnectionInfo();
        String ssid = connectionInfo.getSSID();
        if (Build.VERSION.SDK_INT >= 17 && ssid.startsWith("\"") && ssid.endsWith("\"")) {
            ssid = ssid.substring(1, ssid.length() - 1);
        }

        return ssid;
    }

    private void refreshBtn() {
        if (mSending) {
            mButton.setText("停止发送");
            mProgressBar.setVisibility(View.VISIBLE);
        } else {
            mButton.setText("开始发送");
            mProgressBar.setVisibility(View.INVISIBLE);
        }
    }

    private void refreshDiscoveryBtn() {
        if (mDiscoverying) {
            mDiscoveryBtn.setText("停止搜索");
            mDiscoveryPb.setVisibility(View.VISIBLE);
        } else {
            mDiscoveryBtn.setText("开始搜索");
            mDiscoveryPb.setVisibility(View.INVISIBLE);
        }
    }

    @Override
    protected void onDestroy() {
        DeviceDiscovery.stop();
        WifiConfig.stop();
        super.onDestroy();
    }

    private void allowMulticast(){
        WifiManager wifiManager=(WifiManager)getSystemService(Context.WIFI_SERVICE);

        mMulticastLock = wifiManager.createMulticastLock("multicast.test");

        mMulticastLock.acquire();

//        MulticastSocket socket= null;
//        try {
//            socket = new MulticastSocket(3721);
//        } catch (IOException e) {
//            e.printStackTrace();
//        }
//        socket.joinGroup(this.multicastAddr);
//        socket.setLoopbackMode(false);

    }
}
