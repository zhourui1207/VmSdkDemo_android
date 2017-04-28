package com.joyware.vmnetsdktest;

import android.content.Intent;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;
import android.view.View;

/**
 * Created by zhourui on 17/4/27.
 * email: 332867864@qq.com
 * phone: 17006421542
 */

public class TestStartActivity extends AppCompatActivity {

    GLSurfaceView mGLSurfaceView;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        init();
    }

    private void init() {
        setContentView(R.layout.activity_start_test);

        findViewById(R.id.btn).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                TestStartActivity.this.startActivity(new Intent(TestStartActivity.this, HandlerTestActivity.class));
            }
        });
    }
}
