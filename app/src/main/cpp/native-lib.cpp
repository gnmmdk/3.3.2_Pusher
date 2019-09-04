#include <jni.h>
#include <string>
//extern "C" {//x264和rtmp里面都extern "C"了
#include <x264.h>
#include <rtmp.h>
//}
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
