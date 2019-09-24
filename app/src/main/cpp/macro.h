//
// Created by PC on 2019/9/24.
//

#ifndef INC_3_3_2_PUSHER_MACRO_H
#define INC_3_3_2_PUSHER_MACRO_H

#include <android/log.h>

//定义释放的宏函数
#define DELETE(object) if(object){delete object;object = 0;}
//定义日志打印宏函数
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,"KJJPUSHER",__VA_ARGS__)

#endif //INC_3_3_2_PUSHER_MACRO_H

