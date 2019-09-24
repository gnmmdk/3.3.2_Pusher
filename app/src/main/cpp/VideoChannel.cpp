//
// Created by PC on 2019/9/24.
//


#include "VideoChannel.h"


VideoChannel::VideoChannel() {
    pthread_mutex_init(&mutex,0);
}
VideoChannel::~VideoChannel() {
    pthread_mutex_destroy(&mutex);
}

void VideoChannel::setVideoCallback(VideoChannel::VideoCallback videoCallback) {
    this->videoCallback = videoCallback;
}
/**
 * 初始化x264编码器
 * @param width
 * @param height
 * @param fps
 * @param bitrate
 */
void VideoChannel::initVideoEncoder(int width, int height, int fps, int bitrate) {
    //编码的时候可能会发生宽高的改变，会调用该方法，会只是编码器重复初始化
    //正在使用编码器，产生冲突
    pthread_mutex_lock(&mutex);
    mWidth = width;
    mHeight = height;
    mFps = fps;
    mBitrate = bitrate;
    y_len = width * height;
    uv_len = y_len/4;
    //初始化x264编码器
    x264_param_t param;
    //ultrafast 最快 x264_preset_names[0]
    //zerolatency 零延迟 x264_tune_names[7]
    x264_param_default_preset(&param,x264_preset_names[0]/*"ultrafast"*/,x264_tune_names[7]/*"zerolatency"*/);
    //编码规格，base_line 3.2
    param.i_level_idc = 32;
    //输入数据格式为YUV420P
    param.i_csp = X264_CSP_I420;
    param.i_width = width;
    param.i_height = height;
    //没有b帧 (如果有B帧会音响编码效率）
    param.i_bframe = 0 ;
    //TODO
    pthread_mutex_unlock(&mutex);
}
