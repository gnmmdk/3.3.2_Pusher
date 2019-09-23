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
extern "C"
JNIEXPORT void JNICALL
Java_com_kangjj_pusher_NEPusher_native_1initVideoEncoder(JNIEnv *env, jobject thiz, jint width,
                                                         jint height, jint m_fps, jint bitrate) {
}
extern "C"
JNIEXPORT void JNICALL
Java_com_kangjj_pusher_NEPusher_native_1pushVideo(JNIEnv *env, jobject thiz, jbyteArray data) {
}
extern "C"
JNIEXPORT void JNICALL
Java_com_kangjj_pusher_NEPusher_native_1init(JNIEnv *env, jobject thiz) {
    // TODO: implement native_init()
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