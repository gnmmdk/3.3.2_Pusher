#include <jni.h>
#include <string>
#include <pthread.h>
#include "macro.h"
#include "safe_queue.h"
//extern "C" {//x264和rtmp里面都extern "C"了
#include <x264.h>
#include <rtmp.h>
#include "VideoChannel.h"
//}

VideoChannel *videoChannel = 0;
SafeQueue<RTMPPacket *> packets;
uint32_t start_time;

void releasePackets(RTMPPacket ** packet){
    if(packet){
        RTMPPacket_Free(*packet);
        delete packet;
        packet =0;
    }
}

void callback(RTMPPacket *packet)
{
    if(packet){
        if(packet->m_nTimeStamp == -1){
            packet->m_nTimeStamp = RTMP_GetTime() -start_time;
        }
        packets.push(packet);
    }
}
extern "C" JNIEXPORT jstring JNICALL
Java_com_kangjj_pusher_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    char version[50];
    sprintf(version,"librtmp version:%d",RTMP_LibVersion());
//    return env->NewStringUTF(hello.c_str());
    x264_picture_t* picture = new x264_picture_t;
    return env->NewStringUTF(version);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_kangjj_pusher_NEPusher_native_1init(JNIEnv *env, jobject thiz) {
    //准备编码器进行编码 工具类VideoChannel
    videoChannel = new VideoChannel;
    videoChannel->setVideoCallback(callback);
    packets.setReleaseCallback(releasePackets);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_kangjj_pusher_NEPusher_native_1initVideoEncoder(JNIEnv *env, jobject thiz, jint width,
                                                         jint height, jint m_fps, jint bitrate) {
    if(videoChannel){
        videoChannel->initVideoEncoder(width,height,m_fps,bitrate);
    }
}
extern "C"
JNIEXPORT void JNICALL
Java_com_kangjj_pusher_NEPusher_native_1pushVideo(JNIEnv *env, jobject thiz, jbyteArray data) {
}

extern "C"
JNIEXPORT void JNICALL
Java_com_kangjj_pusher_NEPusher_native_1start(JNIEnv *env, jobject thiz, jstring path) {
    // TODO: implement native_start()
}
extern "C"
JNIEXPORT void JNICALL
Java_com_kangjj_pusher_NEPusher_native_1stop(JNIEnv *env, jobject thiz) {
    // TODO: implement native_stop()
}