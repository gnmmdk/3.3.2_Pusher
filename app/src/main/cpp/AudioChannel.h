//
// Created by PC on 2019/10/1.
//

#ifndef INC_3_3_2_PUSHER_AUDIOCHANNEL_H
#define INC_3_3_2_PUSHER_AUDIOCHANNEL_H

#include <faac.h>
#include <rtmp.h>
#include <sys/types.h>
#include <cstring>
#include "macro.h"

class AudioChannel {
    typedef  void (*AudioCallback)(RTMPPacket *packet);
public:
    AudioChannel();
    ~AudioChannel();

    void setAudioCallback(void (*fun)(RTMPPacket *));

    void initAudioEncoder(int sample_rate, int num_channels);

    int getInputSamples();

    void encodeData(int8_t *data);

    RTMPPacket *getAudioSeqHeader();

private:
    AudioCallback  audioCallback;
    int mChannels;
    faacEncHandle audioEncoder = 0;
    u_long inputSamples;
    u_long maxOutputBytes;
    u_char *buffer = 0 ;
};


#endif //INC_3_3_2_PUSHER_AUDIOCHANNEL_H
