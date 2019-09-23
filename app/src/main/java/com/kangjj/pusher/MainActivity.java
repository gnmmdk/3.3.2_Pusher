package com.kangjj.pusher;

import androidx.appcompat.app.AppCompatActivity;

import android.hardware.Camera;
import android.os.Bundle;
import android.view.SurfaceView;
import android.view.View;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {
//    private CameraHelper cameraHelper;
    private NEPusher pusher;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        SurfaceView surfaceView = findViewById(R.id.surfaceView);
        pusher = new NEPusher(this, Camera.CameraInfo.CAMERA_FACING_BACK,640, 480,25,800000);
        pusher.setPreviewDisplay(surfaceView.getHolder());
//        cameraHelper = new CameraHelper(this, Camera.CameraInfo.CAMERA_FACING_BACK,480,800);
//        cameraHelper.setPreviewDisplay(surfaceView.getHolder());
    }

    public void switchCamera(View view) {
//        cameraHelper.switchCamera();
        pusher.switchCamera();
    }

    public void startLive(View view) {
        pusher.startLive("rtmp");
    }

    public void stopLive(View view) {
        pusher.stopLive();
    }
}
