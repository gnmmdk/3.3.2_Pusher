//
// Created by PC on 2019/10/1.
//


#include "AudioChannel.h"


AudioChannel::AudioChannel() {

}

AudioChannel::~AudioChannel() {
    DELETE(buffer);
    if(audioEncoder){
        faacEncClose(audioEncoder);
        audioEncoder = 0 ;
    }
}

void AudioChannel::setAudioCallback(AudioCallback audioCallback) {
    this->audioCallback = audioCallback;
}

void AudioChannel::initAudioEncoder(int sample_rate, int channels) {
    mChannels = channels;
    /**
     * 第一步 打开编码器
     * inputSamples：编码器一次最大能接受的样本数（一个样本16bit，2字节）
     * maxOutputBytes：编码器最大能输出数据的字节数
     */
    audioEncoder = faacEncOpen(sample_rate,channels,&inputSamples,&maxOutputBytes);
    if(!audioCallback){
        LOGE("打开音频编码器失败");
        return;
    }
    //第二步：配置编码器参数
    faacEncConfigurationPtr config = faacEncGetCurrentConfiguration(audioEncoder);
    //mpeg4标准
    config->mpegVersion = MPEG4;
    //LC标准
    config->aacObjectType = LOW;
    //16位
    config->inputFormat = FAAC_INPUT_16BIT;
    //编码出来的是原始数据，不是ADTS
    config->outputFormat = 0 ;
    //噪声控制（降噪）
    config->useTns = 1;
    config->useLfe = 0;
    int ret = faacEncSetConfiguration(audioEncoder,config);
    if(!ret){
        LOGE("音频编码器参数配置失败");
        return;
    }
    //输出缓冲区
    buffer = new u_char[maxOutputBytes];
}

int AudioChannel::getInputSamples() {
    return inputSamples;
}

void AudioChannel::encodeData(int8_t *data) {
    //返回编码后数据的字节长度
    int byteLen = faacEncEncode(audioEncoder, reinterpret_cast<int32_t *>(data), inputSamples, buffer, maxOutputBytes);
    if(byteLen>0){
        //看图表，拼数据
        RTMPPacket *packet = new RTMPPacket;
        int body_size = 2+ byteLen;//参考图表
        RTMPPacket_Alloc(packet,body_size);
        packet->m_body[0] = 0xAF;//双声道（对照Flv.md文件）
        if(mChannels == 1){
            packet->m_body[0] = 0xAE;//单声道
        }
        //这里是编码出来的音频数据，所以都是01
        packet->m_body[1] = 0x01;
        //音频数据
        memcpy(&packet->m_body[2],buffer,byteLen);
        packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;//包类型 音频
        packet->m_nBodySize = body_size;
        packet->m_nChannel = 11;
        packet->m_nTimeStamp = -1;//帧数据有时间戳
        packet->m_hasAbsTimestamp = 0;
        packet->m_headerType = RTMP_PACKET_SIZE_LARGE;

        audioCallback(packet);
    }
}

RTMPPacket *AudioChannel::getAudioSeqHeader() {
    u_char *ppBuffer;
    u_long len;
    faacEncGetDecoderSpecificInfo(audioEncoder,&ppBuffer,&len);
    RTMPPacket *packet = new RTMPPacket;
    int body_size = 2+len;
    RTMPPacket_Alloc(packet,body_size);
    packet->m_body[0]=0xAF;
    if(mChannels==1){
        packet->m_body[1] = 0xAE;//单声道
    }
    //这里是序列头，所以是00
    packet->m_body[1] = 0x00;
    memcpy(&packet->m_body[2],ppBuffer,len);
    packet->m_nBodySize = body_size;
    packet->m_nChannel = 1;
    packet->m_nTimeStamp = 0;//头数据不需要时间戳
    packet->m_hasAbsTimestamp = 0 ;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;

    return packet;
}
