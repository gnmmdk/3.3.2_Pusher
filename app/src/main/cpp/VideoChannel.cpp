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
    pthread_mutex_lock(&mutex);
    //y数据
    memcpy(pic_in->img.plane[0],data,y_len);
    for(int i = 0;i<uv_len;++i){
        //u 数据 参考yuv彩色表
        *(pic_in->img.plane[1]+i) = *(data +y_len + i*2 +1);
        //v 数据 参考yuv彩色表
        *(pic_in->img.plane[2]+i) = *(data+y_len+i*2);
    }
    //通过H.264编码得到NAL数组
    x264_nal_t *nal = 0 ;
    int pi_nal;
    x264_picture_t pic_out;
    //进行编码
    int ret = x264_encoder_encode(videoEncoder,&nal,&pi_nal,pic_in,&pic_out);
    if(ret < 0){
        LOGE("x264编码失败");
        pthread_mutex_unlock(&mutex);
        return;
    }
    //sps pps
    int sps_len,pps_len;
    uint8_t sps[100];
    uint8_t pps[100];
    pic_in->i_pts += 1;
    for (int i = 0; i < pi_nal; ++i) {
        if(nal[i].i_type == NAL_SPS){
            sps_len = nal[i].i_payload - 4;//去掉起始码 是否需要判断-3？
            memcpy(sps,nal[i].p_payload+4,sps_len);
        }else if(nal[i].i_type ==NAL_PPS){
            pps_len = nal[i].i_payload - 4;//去掉起始码
            memcpy(pps,nal[i].p_payload + 4,pps_len);
            //pps是跟在sps后面，这里达到pps表示前面sps肯定已经拿到了
            sendSpsPps(sps,pps,sps_len,pps_len);
        }else{
            //帧类型
            sendFrame(nal[i].i_type,nal[i].i_payload,nal[i].p_payload);
        }
    }

    pthread_mutex_unlock(&mutex);
}
/**
 * 发送sps pps包
 * @param sps
 * @param pps
 * @param len
 * @param ppsLen
 */
void VideoChannel::sendSpsPps(uint8_t *sps, uint8_t *pps, int sps_len, int pps_len) {
    RTMPPacket *packet = new RTMPPacket;
    //参考RTMPDump与X264.md的表
    int body_size = 5 + 8 +sps_len + 3+ pps_len;

    RTMPPacket_Alloc(packet,body_size);
    int i = 0;
    packet->m_body[i++] = 0x17;

    packet->m_body[i++] = 0x00;
    packet->m_body[i++] = 0x00;
    packet->m_body[i++] = 0x00;
    packet->m_body[i++] = 0x00;

    packet->m_body[i++] = 0x01;
    packet->m_body[i++] = sps[1];
    packet->m_body[i++] = sps[2];
    packet->m_body[i++] = sps[3];

    packet->m_body[i++] = 0xFF;
    packet->m_body[i++] = 0xE1;

    packet->m_body[i++] = (sps_len >> 8)& 0xFF;//?
    packet->m_body[i++] = sps_len & 0xFF;//?

    memcpy(&packet->m_body[i],sps,sps_len);

    i+=sps_len;//拷贝完sps数据 ，i移位
    packet->m_body[i++] =0x01;
    packet->m_body[i++] = (pps_len >> 8)& 0xFF;//?
    packet->m_body[i++] = pps_len & 0xFF;//?

    memcpy(&packet->m_body[i],pps,pps_len);
    i+=pps_len;//拷贝完sps数据 ，i移位
    //包类型
    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nBodySize = body_size;
    packet->m_nChannel = 10;
    packet->m_nTimeStamp = 0;//sps pps包 没有时间戳
    packet->m_hasAbsTimestamp = 0 ;
    packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    //把数据包放入队列
    videoCallback(packet);
}
/**
 * 发送帧数据
 * @param type
 * @param payload
 * @param pPayload
 */
void VideoChannel::sendFrame(int type, int payload, uint8_t *pPayload) {
//   去掉起始码 00 00 00 01 或者 00 00 01
    if(pPayload[2] ==0x00){
        pPayload += 4;
        payload -= 4;
    }else if(pPayload[2]==0x01){
        pPayload += 3;
        payload -= 3;
    }

    RTMPPacket *packet = new RTMPPacket;
    int body_size = 5 + 4 +payload;//参考图表
    RTMPPacket_Alloc(packet,body_size);
    packet->m_body[0] = 0x27;//非关键帧
    if(type == NAL_SLICE_IDR){
        packet->m_body[0] = 0x17;//关键帧
    }
    packet->m_body[1] = 0x01;
    packet->m_body[2] = 0x00;
    packet->m_body[3] = 0x00;
    packet->m_body[4] = 0x00;

    packet->m_body[5] = (payload >> 24) & 0xFF;
    packet->m_body[6] = (payload >> 16) & 0xFF;
    packet->m_body[7] = (payload >> 8) & 0xFF;
    packet->m_body[8] = payload & 0xFF;

    memcpy(&packet->m_body[9], pPayload, payload);
    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;//包类型
    packet->m_nBodySize = body_size;
    packet->m_nChannel = 10;
    packet->m_nTimeStamp = -1;//外部设置时间
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    //把数据包放入队列
    videoCallback(packet);
}
