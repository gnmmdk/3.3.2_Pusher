package com.kangjj.pusher;

import android.app.Activity;
import android.hardware.Camera;
import android.view.SurfaceHolder;

public class VideoChannel implements Camera.PreviewCallback,CameraHelper.OnChangedSizeListener{
    private CameraHelper cameraHelper;
    private int bitrate;
    private int mFps;
    private boolean isLive;
    private NEPusher mPusher;

    public VideoChannel(NEPusher pusher, Activity activity, int cameraId, int width, int height,
                        int fps, int bitrate) {
        this.mFps = fps;
        this.mPusher = pusher;
        this.bitrate = bitrate;
        cameraHelper = new CameraHelper(activity,cameraId,width,height);
        cameraHelper.setPreviewCallback(this);
        cameraHelper.setOnChangedSizeListener(this);
    }

    public void setPreviewDisplay(SurfaceHolder holder){
        cameraHelper.setPreviewDisplay(holder);
    }

    public void switchCamera(){
        cameraHelper.switchCamera();
    }

    public void startLive() {
        isLive = true;
    }

    public void stopLive() {
        isLive = false;
    }

    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {
        if(isLive){
            //图像数据推送
            mPusher.native_pushVideo(data);
        }
    }

    @Override
    public void onChanged(int width, int height) {
        mPusher.native_initVideoEncoder(width,height,mFps,bitrate);
    }

    public void release() {
        cameraHelper.stopPreview();
    }
}
