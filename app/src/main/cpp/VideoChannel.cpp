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

    if(videoEncoder){
        x264_encoder_close(videoEncoder);
        videoEncoder = 0 ;
    }
    if(pic_in)
    {
        x264_picture_clean(pic_in);
        DELETE(pic_in);
    }
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
    //码率控制方式。
    // #define X264_RC_CQP      恒定质量
    //#define X264_RC_CRF       恒定码率
    //#define X264_RC_ABR       平均码率
    param.rc.i_rc_method = X264_RC_CRF;
    //码率(比特率，单位Kb/s）
    param.rc.i_bitrate = bitrate/1000;
    //瞬间最大码率
    param.rc.i_vbv_max_bitrate = bitrate / 1000 *1.2;
    //设置了i_vbv_max_bitrate就必须设置buffer大小，码率控制区大小，单位Kb/s
    param.rc.i_vbv_buffer_size = bitrate / 1000;
    //码率控制不是通过timebase 和timestamp,而是空过fps
    param.b_vfr_input = 0;
    //帧率分子
    param.i_fps_num = fps;
    //帧率分母
    param.i_fps_den = 1;
    param.i_timebase_den = param.i_fps_num;
    param.i_timebase_num = param.i_fps_den;
    //帧距离（关键帧) 2s一个关键帧
    param.i_keyint_max = fps * 2;
    //是否复制sps和pps放在每个关键帧前面 该参数设置是让每个关键帧（I帧）都附带sps/pps
    param.b_repeat_headers = 1;
    //并行编码线程数
    param.i_threads = 1 ;
    //profile级别，baseline级别
    x264_param_apply_profile(&param,"baseline");
    //输入图像初始化
    pic_in = new x264_picture_t;
    x264_picture_alloc(pic_in,param.i_csp,param.i_width,param.i_height);
    //打开编码器
    videoEncoder = x264_encoder_open(&param);
    if(videoEncoder){
        LOGE("x264编码器打开成功");
    }

    pthread_mutex_unlock(&mutex);
}

void VideoChannel::encodeData(int8_t *data) {
    //TODO

}
