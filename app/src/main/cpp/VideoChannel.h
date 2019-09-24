//
// Created by PC on 2019/9/24.
//

#ifndef INC_3_3_2_PUSHER_VIDEOCHANNEL_H
#define INC_3_3_2_PUSHER_VIDEOCHANNEL_H

#include <pthread.h>
#include <rtmp.h>
#include <x264.h>

class VideoChannel {
typedef void (*VideoCallback)(RTMPPacket *packet);
public:
    VideoChannel();
    virtual  ~VideoChannel();

    void setVideoCallback(VideoCallback videoCallback);

    void initVideoEncoder(int width, int height, int fps, int bitrate);

private:
    pthread_mutex_t mutex;
    VideoCallback  videoCallback;
    int mWidth;
    int mHeight;
    int mFps;
    int mBitrate;
    int y_len;
    int uv_len;
};


#endif //INC_3_3_2_PUSHER_VIDEOCHANNEL_H
