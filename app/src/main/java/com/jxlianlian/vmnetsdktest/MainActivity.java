package com.jxlianlian.vmnetsdktest;

import android.opengl.GLSurfaceView;
import android.os.AsyncTask;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.jxlianlian.masdk.Channel;
import com.jxlianlian.masdk.ChannelsHolder;
import com.jxlianlian.masdk.DepTree;
import com.jxlianlian.masdk.DepTreesHolder;
import com.jxlianlian.masdk.VmNet;
import com.jxlianlian.masdk.VmPlayer;
import com.jxlianlian.masdk.util.OpenGLESUtil;
import com.jxlianlian.masdk.util.opengles.GLFrameRenderer;

import java.util.List;

public class MainActivity extends AppCompatActivity implements VmNet.ServerConnectStatusCallback {

  private String TAG = "MainActivity";

  private SurfaceView surfaceView;
  private GLSurfaceView glSurfaceView;
  GLFrameRenderer mGLFRenderer;
  private Button button;
  private Button record;

  public GLFrameRenderer getGLFrameRenderer() {
    return mGLFRenderer;
  }

  boolean audioOpen = false;

  boolean recording = false;

  VmPlayer player;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);

    surfaceView = (SurfaceView) findViewById(R.id.surfaceView);
    button = (Button) findViewById(R.id.button);
    glSurfaceView = (GLSurfaceView) findViewById(R.id.glSurfaceView);

    button.setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View view) {
        if (audioOpen) {
          player.closeAudio();
          audioOpen = false;
          button.setText("打开声音");
        } else {
          audioOpen = player.openAudio();
          if (audioOpen) {
            button.setText("关闭声音");
          }
        }

      }
    });

    record = (Button) findViewById(R.id.record);
    record.setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View view) {
        if (recording) {
          player.stopRecord();
          recording = false;
          record.setText("开始录像");
        } else {
          recording = player.startRecord("/sdcard/MVSS/Localrecord/test1.flv");
          if (recording) {
            record.setText("停止录像");
          }
        }
      }
    });

    // Example of a call to a native method
    TextView tv = (TextView) findViewById(R.id.sample_text);

    boolean supportOpenGLES = OpenGLESUtil.detectOpenGLES20(this);
    if (supportOpenGLES) {
      tv.setText("支持openGLES2.0");
    } else {
      tv.setText("不支持openGLES2.0");
    }
    glSurfaceView.setEGLContextClientVersion(2);
    mGLFRenderer = new GLFrameRenderer(glSurfaceView);
    glSurfaceView.setRenderer(mGLFRenderer);


    new RealplayTask().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
  }

  @Override
  public void onServerConnectStatus(boolean isConnected) {
    Log.e(TAG, "服务器连接状态:" + isConnected);
  }

  class RealplayTask extends AsyncTask<Void, Void, Void> {

    @Override
    protected Void doInBackground(Void... voids) {
      VmNet.init(10);
//      VmNet.connect("192.168.1.113", 5516, MainActivity.this);
      VmNet.connect("118.178.132.146", 5516, MainActivity.this);
      try {
        Thread.sleep(2000);
      } catch (InterruptedException e) {
        e.printStackTrace();
      }
      int ret = VmNet.login("admin", "123456");
      if (ret == 0) {
        Log.e(TAG, "登录成功!");
        DepTreesHolder depTreesHolder = new DepTreesHolder();
        ret = VmNet.getDepTrees(1, 100, depTreesHolder);
        if (ret == 0 && depTreesHolder.size() > 0) {
          List<DepTree> depTrees = depTreesHolder.list();
          for (DepTree depTree : depTrees) {
            Log.e(TAG, "depName=" + depTree.getDepName() + ", depId=" + depTree.getDepId() + ", parentId=" + depTree.getParentId());
          }
        }

        ChannelsHolder channelsHolder = new ChannelsHolder();
        ret = VmNet.getChannels(1, 100, 0, channelsHolder);
        if (ret == 0 && channelsHolder.size() > 0) {
          List<Channel> channels = channelsHolder.list();
          for (Channel channel : channels) {
            Log.e(TAG, "channelName=" + channel.getChannelName() + ", fdId=" + channel.getFdId() + ", channelId=" + channel.getChannelId());
          }
        }

//      VmNet.sendControl("201610111654538071", 1, 3, 5, 0);
//
//      try {
//        Thread.sleep(5000);
//      } catch (InterruptedException e) {
//        e.printStackTrace();
//      }
//
//      VmNet.sendControl("201610111654538071", 1, 0, 0, 0);
//
//      try {
//        Thread.sleep(2000);
//      } catch (InterruptedException e) {
//        e.printStackTrace();
//      }
//
//      VmNet.sendControl("201610111654538071", 1, 4, 5, 0);
//
//      try {
//        Thread.sleep(5000);
//      } catch (InterruptedException e) {
//        e.printStackTrace();
//      }
//
//      VmNet.sendControl("201610111654538071", 1, 0, 0, 0);
//
//      try {
//        Thread.sleep(5000);
//      } catch (InterruptedException e) {
//        e.printStackTrace();
//      }


//      PlayAddressHolder playAddress = new PlayAddressHolder();
//      ret = VmNet.openRealplayStream("201610111654538071", 1, true, playAddress);
//      if (ret == 0) {
//        Log.e(TAG, "monitorId:" + playAddress.getMonitorId() + ",videoAddr:" + playAddress.getVideoAddr() +  ",videoPort:" + playAddress.getVideoPort());
//        StreamIdHolder streamIdHolder = new StreamIdHolder();
//        ret = VmNet.startStream(playAddress.getVideoAddr(), playAddress.getVideoPort(), new VmNet.StreamCallback() {
//
//          @Override
//          public void onStreamConnectStatus(int streamId, boolean isConnected) {
//            Log.e(TAG, "码流" + streamId + "状态:" + isConnected);
//          }
//
//          @Override
//          public void onReceiveStream(int streamId, int streamType, int payloadType, byte[] buffer, int timeStamp, int seqNumber, boolean isMark) {
//            Log.e(TAG, "接收到码流 streamId=" + streamId + ", streamType=" + streamType + ", payloadType=" + payloadType + ", len=" + buffer.length + ", isMark=" + isMark);
//          }
//        }, streamIdHolder);
//        if (ret == 0) {
//          Log.e(TAG, "开始播放成功");
//          try {
//            Thread.sleep(100000000);
//          } catch (InterruptedException e) {
//            e.printStackTrace();
//          }
//          VmNet.stopStram(streamIdHolder.getStreamId());
//        }
//
//        VmNet.closeRealplayStream(playAddress.getMonitorId());
//
//      } else {
//        Log.e(TAG, "ret=" + ret);
//      }
      } else {
        Log.e(TAG, "登录失败!");
      }
      return null;
    }

    @Override
    protected void onPostExecute(Void aVoid) {
      player = new VmPlayer();
      player.startRealplay("201607201402389091", 1, true, 0, false, glSurfaceView.getHolder(), MainActivity.this);
//      player.startRealplay("201610111654538071", 1, true, 2, false, glSurfaceView.getHolder(), MainActivity.this);

    }
  }
}
