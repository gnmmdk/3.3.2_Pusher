//
// Created by PC on 2019/9/24.
//

#ifndef INC_3_3_2_PUSHER_VIDEOCHANNEL_H
#define INC_3_3_2_PUSHER_VIDEOCHANNEL_H

#include <pthread.h>
#include <rtmp.h>
#include <x264.h>
#include "macro.h"

class VideoChannel {
typedef void (*VideoCallback)(RTMPPacket *packet);
public:
    VideoChannel();
    virtual  ~VideoChannel();

    void setVideoCallback(VideoCallback videoCallback);

    void initVideoEncoder(int width, int height, int fps, int bitrate);

    void encodeData(int8_t *data);

private:
    pthread_mutex_t mutex;
    VideoCallback  videoCallback;
    int mWidth;
    int mHeight;
    int mFps;
    int mBitrate;
    int y_len;
    int uv_len;
    x264_picture_t *pic_in = 0;
    x264_t * videoEncoder = 0 ;

    void sendSpsPps(uint8_t *sps, uint8_t *pps, int len, int ppsLen);

    void sendFrame(int type, int payload, uint8_t *pPayload);
};


#endif //INC_3_3_2_PUSHER_VIDEOCHANNEL_H
