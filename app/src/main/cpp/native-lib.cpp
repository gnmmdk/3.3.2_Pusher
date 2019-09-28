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
bool isStart;
bool readyPushing;
pthread_t pid_start;

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

void* task_start(void* args){
    char * url = static_cast<char *>(args);
    RTMP *rtmp = 0;
    int ret;
    do{
        //1.1 rtmp 初始化
        rtmp= RTMP_Alloc();
        if(!rtmp){
            LOGE("rtmp 初始化失败");
            break;
        }
        //1.2 rtmp初始化
        RTMP_Init(rtmp);
        //设置连接的超时时间
        rtmp->Link.timeout = 5;
        //2 设置流媒体地址
        ret = RTMP_SetupURL(rtmp,url);
        if(!ret){
            LOGE("rtmp 设置流媒体地址失败");
            break;
        }
        //3 开始输出模式
        RTMP_EnableWrite(rtmp);
        //4 建立连接
        ret = RTMP_Connect(rtmp,0);
        if(!ret){
            LOGE("rtmp 建立连接失败:%d, url: %s", ret, url);
            break;
        }
        //准备好了，可以开始向服务器推流
        readyPushing = 1;
        //记录开始推流的时间戳
        start_time = RTMP_GetTime();
        //后面要对安全队列进行取数据的操作
        packets.setWork(1);
        RTMPPacket * packet = 0;
        //循环从队列中取数据（rtmp包），然后发送
        while (readyPushing){
            packets.pop(packet);
            if(!readyPushing){
                break;
            }
            if(!packet){
                continue;
            }
            //成功取到数据包，发送
            //给一个rtmp的流id
            packet->m_nInfoField2 = rtmp->m_stream_id;
            //将true放入队列
            ret = RTMP_SendPacket(rtmp,packet,1);
            releasePackets(&packet);
            if(!ret){
                LOGE("rtmp 断开");
                break;
            }
        }
        releasePackets(&packet);
    }while(0);
    isStart = 0;
    readyPushing = 0 ;
    packets.setWork(0);
    packets.clear();
    //释放rtmp
    if(rtmp){
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
    }
    delete(url);
    return 0;
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
Java_com_kangjj_pusher_NEPusher_native_1pushVideo(JNIEnv *env, jobject thiz, jbyteArray data_) {
    if(!videoChannel || !readyPushing){
        return;
    }

    jbyte *data = env->GetByteArrayElements(data_,NULL);

    videoChannel->encodeData(data);
    env->ReleaseByteArrayElements(data_,data,0);

}

extern "C"
JNIEXPORT void JNICALL
Java_com_kangjj_pusher_NEPusher_native_1start(JNIEnv *env, jobject thiz, jstring path_) {
    if(isStart){
        return;
    }
    isStart = 1;
    const char *path = env->GetStringUTFChars(path_,0);
    char *url = new char(strlen(path)+1);
    strcpy(url,path);
    //创建线程进行直播
    pthread_create(&pid_start, 0, task_start, url);
    env->ReleaseStringUTFChars(path_,path);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_kangjj_pusher_NEPusher_native_1stop(JNIEnv *env, jobject thiz) {
    isStart = 0;
    readyPushing = 0;
    packets.setWork(0);
    pthread_join(pid_start,0);
}