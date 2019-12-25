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
 * todo A.3 初始化x264编码器
 * @param width
 * @param height
 * @param fps
 * @param bitrate
 */
void VideoChannel::initVideoEncoder(int width, int height, int fps, int bitrate) {
    //todo 编码的时候可能会发生宽高的改变，会调用该方法，会只是编码器重复初始化
    // 正在使用编码器，产生冲突
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
    //todo A.3.1 初始化x264编码器以及设置参数
    x264_param_t param;
    //ultrafast 最快 x264_preset_names[0] 最快编码速度
    //zerolatency 零延迟 x264_tune_names[7]
    x264_param_default_preset(&param,x264_preset_names[0]/*"ultrafast"*/,x264_tune_names[7]/*"zerolatency"*/);
    //todo A.3.1.1 编码规则，base_line 3.2
    param.i_level_idc = 32;
    //todo A.3.1.2 输入数据格式为YUV420P
    param.i_csp = X264_CSP_I420;
    param.i_width = width;
    param.i_height = height;
    //todo A.3.1.3 没有b帧 (如果有B帧会影响编码效率）
    param.i_bframe = 0 ;
    //todo A.3.1.4 码率控制方式。

    // #define X264_RC_CQP      恒定质量
    //#define X264_RC_CRF       恒定码率
    //#define X264_RC_ABR       平均码率
    param.rc.i_rc_method = X264_RC_CRF;
    //码率(比特率，单位Kb/s）
    param.rc.i_bitrate = bitrate/1000;
    //todo A.3.1.5 瞬间最大码率
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
    //todo A.3.1.6 帧距离（关键帧) 2s一个关键帧
    param.i_keyint_max = fps * 2;
    //todo A.3.1.7 是否复制sps和pps放在每个关键帧前面 该参数设置是让每个关键帧（I帧）都附带sps/pps
    param.b_repeat_headers = 1;
    //并行编码线程数
    param.i_threads = 1 ;
    //todo A.3.1.8 profile级别，baseline级别
    x264_param_apply_profile(&param,"baseline");
    //todo A.3.2  输入图像初始化
    pic_in = new x264_picture_t;//下方的encodeData有用到
    x264_picture_alloc(pic_in,param.i_csp,param.i_width,param.i_height);//param.i_csp = X264_CSP_I420;
    //todo A.3.3 打开编码器
    videoEncoder = x264_encoder_open(&param);
    if(videoEncoder){
        LOGE("x264编码器打开成功");
    }

    pthread_mutex_unlock(&mutex);
}
//todo A.4 NV21转I420(这两个都属于YUV420格式)，然后进行编码  Java_com_kangjj_pusher_NEPusher_native_1pushVideo调用进来。
void VideoChannel::encodeData(int8_t *data) {
    pthread_mutex_lock(&mutex);
    //todo A.4.1 NV21转I420
    //todo A.4.1.1 y数据 都一样，所以直接拷贝
    memcpy(pic_in->img.plane[0],data,y_len);
    for(int i = 0;i<uv_len;++i){
        ///todo A.4.1.2 u 数据 参考yuv彩色表 *是解引用 取值 pic_in->img.plane[1] 是指针 "+y_len" 是因为前面的是y的数据
        *(pic_in->img.plane[1]+i) = *(data +y_len + i*2 +1);
        //todo A.4.1.3 v 数据 参考yuv彩色表
        *(pic_in->img.plane[2]+i) = *(data+y_len+i*2);
    }
    //todo NALU就是NAL UNIT，nal单元。NAL全称Network Abstract Layer, 即网络抽象层，
    // H.264在网络上传输的结构。一帧图片经过 H.264 编码器之后，就被编码为一个或多个片
    // （slice），而装载着这些片（slice）的载体，就是 NALU 了
    //todo A.4.2 通过H.264编码得到NAL数组 x264_encoder_encode
    x264_nal_t *nal = 0 ;
    int pi_nal;
    x264_picture_t pic_out;
    //todo 进行编码
    int ret = x264_encoder_encode(videoEncoder,&nal,&pi_nal,pic_in,&pic_out);
    if(ret < 0){
        LOGE("x264编码失败");
        pthread_mutex_unlock(&mutex);
        return;
    }
    //sps pps https://www.jianshu.com/p/4a967b9a472d
    int sps_len,pps_len;
    uint8_t sps[100];
    uint8_t pps[100];
    pic_in->i_pts += 1;
    for (int i = 0; i < pi_nal; ++i) {
        if(nal[i].i_type == NAL_SPS){//todo A.4.2  获取sps数据
            //   去掉起始码 00 00 00 01 或者 00 00 01 TODO
//            if(pPayload[2] ==0x00){
//                pPayload += 4;
//                payload -= 4;
//            }else if(pPayload[2]==0x01){
//                pPayload += 3;
//                payload -= 3;
//            }
            sps_len = nal[i].i_payload - 4;//去掉起始码 是否需要判断-3？
            memcpy(sps,nal[i].p_payload+4,sps_len);
        }else if(nal[i].i_type ==NAL_PPS){//todo A.4.3  获取到pps数据 ，然后将sps和pps发送出去
            pps_len = nal[i].i_payload - 4;//去掉起始码
            memcpy(pps,nal[i].p_payload + 4,pps_len);
            //todo pps是跟在sps后面，这里达到pps表示前面sps肯定已经拿到了
            sendSpsPps(sps,pps,sps_len,pps_len);
        }else{//todo A.4.4  发送帧数据
            //帧类型
            sendFrame(nal[i].i_type,nal[i].i_payload,nal[i].p_payload);
        }
    }

    pthread_mutex_unlock(&mutex);
}
/**
 *  //todo A.4.3 发送sps pps包
 * @param sps
 * @param pps
 * @param len
 * @param ppsLen
 */
void VideoChannel::sendSpsPps(uint8_t *sps, uint8_t *pps, int sps_len, int pps_len) {
    RTMPPacket *packet = new RTMPPacket;
    //参考RTMPDump与X264.md的表拷贝数据
    int body_size = 5 + 8 +sps_len + 3+ pps_len;
    //todo A.4.3.1 初始化RTMPPacket
    RTMPPacket_Alloc(packet,body_size);
    //todo A.4.3.2  拷贝内容到packet->m_body[]
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

    packet->m_body[i++] = (sps_len >> 8)& 0xFF;// 取高八位?
    packet->m_body[i++] = sps_len & 0xFF;//        低八位?

    memcpy(&packet->m_body[i],sps,sps_len);

    i+=sps_len;//拷贝完sps数据 ，i移位
    packet->m_body[i++] =0x01;
    packet->m_body[i++] = (pps_len >> 8)& 0xFF;//?
    packet->m_body[i++] = pps_len & 0xFF;//?

    memcpy(&packet->m_body[i],pps,pps_len);
    i+=pps_len;//拷贝完sps数据 ，i移位
    //todo A.4.3.3 packet设置参数
    //包类型
    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nBodySize = body_size;
    packet->m_nChannel = 10;
    packet->m_nTimeStamp = 0;//todo sps pps包 没有时间戳
    packet->m_hasAbsTimestamp = 0 ;
    packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    //todo A.4.3.4 把数据包放入队列 进行发送（生产者 -> 消费者）
    videoCallback(packet);
}
/**
 * //todo A.4.4 发送帧数据
 * @param type
 * @param payload
 * @param pPayload
 */
void VideoChannel::sendFrame(int type, int payload, uint8_t *pPayload) {
    //todo 往RTMP包中填充的是H.264数据，但不是直接将x264编码出来的数据填充进去。
    // 一段包含了N个图像的H.264裸数据，每个NAL之间由：
    //	00 00 00 01 或者 00 00 01
    // 进行分割。在分割符之后的第一个字节，就是表示这个nal的类型。
    // - 0x67：sps
    // - 0x68：pps
    // - 0x65：IDR
    //todo A.4.4.1 去掉起始码 00 00 00 01 或者 00 00 01
    if(pPayload[2] ==0x00){
        pPayload += 4;
        payload -= 4;
    }else if(pPayload[2]==0x01){
        pPayload += 3;
        payload -= 3;
    }

    //todo A.4.4.2 初始化RTMPPacket
    RTMPPacket *packet = new RTMPPacket;
    int body_size = 5 + 4 +payload;//参考图表
    RTMPPacket_Alloc(packet,body_size);
    //todo A.4.4.3 关键帧是0x17 非关键帧是0x27
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
    //todo A.4.4.4 packet的其他属性
    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;//包类型
    packet->m_nBodySize = body_size;
    packet->m_nChannel = 10;
    packet->m_nTimeStamp = -1;//外部设置时间
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    //todo A.4.3.5 把数据包放入队列 进行发送（生产者 -> 消费者）
    videoCallback(packet);
}
