# RTMPDump与X264

标签（空格分隔）： ndk 直播

---
[TOC]
## RTMPDump
[RTMPDump][1] 是一个用来处理RTMP流媒体的开源工具包。它能够单独使用进行RTMP的通信，也可以集成到FFmpeg中通过FFmpeg接口来使用RTMPDump。

> 源码下载：[http://rtmpdump.mplayerhq.hu/download][2]

在Android中可以直接借助NDK在JNI层调用RTMPDump来完成RTMP通信。

> 在根目录下提供了一个Makefile与一些.c源文件。这里的源文件将会编译出一系列的可执行文件。然后我们需要的并不是可执行文件，真正的对RTMP的实现都在librtmp子目录中。在这个子目录中同样包含了一个Makefile文件。通过阅读Makefile发现，它的源码并不多:OBJS=rtmp.o log.o amf.o hashswf.o parseurl.o。因此我们不进行预编译，即直接放入AS中借助CMakeLists.txt来进行编译。这么做可以让我们方便的对库本身进行调试或修改(实际上我们确实会稍微修改这个库的源码)。

在AS中复制librtmp置于:src/main/cpp/librtmp，并为其编写CMakeLists.txt：

```cmake
#所有源文件放入 rtmp_src 变量
file(GLOB rtmp_src *.c)
#编译静态库
add_library(rtmp STATIC ${rtmp_src} )
```
在app/CMakeLists.txt中导入这个CMakeLists.txt：
```cmake
cmake_minimum_required(VERSION 3.4.1)
#引入指定目录下的CMakeLists.txt
add_subdirectory(librtmp)
#指定头文件查找路径
include_directories(librtmp)
add_library(native-lib SHARED native-lib.cpp)
target_link_libraries(native-lib rtmp log)
```
尝试编译：
```
xxx/app/src/main/cpp/librtmp/rtmp.c:40:10: fatal error: 'openssl/ssl.h' file not found
#include <openssl/ssl.h>
         ^~~~~~~~~~~~~~~
1 error generated.
```
问题排查：
打开rtmp.c，我们发现有这里一段宏定义：
```c++
#ifdef CRYPTO
#ifdef USE_POLARSSL
#include <polarssl/havege.h>
#elif defined(USE_GNUTLS)
#include <gnutls/gnutls.h>
#else	/* USE_OPENSSL */
#include <openssl/ssl.h>
#include <openssl/rc4.h>
#endif
TLS_CTX RTMP_TLS_ctx;
#endif
```
最终只有 CRYPTO 这个宏被定义了才会 `#include <openssl/ssl.h>`。那么我们继续查找 CRYPTO 定义的地方，在 rtmp.h 中又有这样一段：
```c++
#if !defined(NO_CRYPTO) && !defined(CRYPTO)
#define CRYPTO
#endif
```
我们只需要编译时添加定义 NO_CRYPTO 这个预编译宏就可以了。修改 librtmp中的CMakelists.txt：
```
cmake_minimum_required(VERSION 3.4.1)
#预编译宏
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNO_CRYPTO")
#所有源文件放入 rtmp_src 变量
file(GLOB rtmp_src *.c)
#编译静态库
add_library(rtmp STATIC ${rtmp_src})
```
输出版本信息，编译测试：
```c++
#include <jni.h>
#include <string>
#include <rtmp.h>

extern "C" JNIEXPORT jstring JNICALL
Java_com_netease_pusher_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    char version[100];
    sprintf(version, "librtmp version: %d", RTMP_LibVersion());
    return env->NewStringUTF(version);
}
```
输出 `rtmp version: 131840`，131840 对应的16进制为：0x020300，也就是2.3版本。

## RTMP视频数据格式
RTMP视频流格式与 **FLV** 很相似，通过查看FLV的格式文档，就能够知道RTMP视频数据应该怎么拼接。
RTMP中的数据就是由FLV的TAG中的数据区构成。

> 参考FLV.md

一般情况下，组装的RTMPPacket(RTMPDump中的结构体)为：
![视频包](图片\视频包.png)



![视频解码序列包](图片\视频解码序列包.png)

## x264
[x264][3] 是一个C语言编写的目前对H.264标准支持最完善的编解码库。与RTMPDump一样，可以在Android中直接使用，也可以集成进入FFMpeg。

### 交叉编译
在linux下下载编译。

下载：`wget ftp://ftp.videolan.org/pub/x264/snapshots/last_x264.tar.bz2`
解压： `tar -xvf last_x264.tar.bz2`
查看configure文件，编写编译脚本(基本与ffmpeg类似):

```
#!/bin/bash

NDK_ROOT=/root/android-ndk-r17c

PREFIX=./android/armeabi-v7a

TOOLCHAIN=$NDK_ROOT/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64

CFLAGS="-isysroot $NDK_ROOT/sysroot -isystem $NDK_ROOT/sysroot/usr/include/arm-linux-androideabi -D__ANDROID_API__=17 -g -DANDROID -ffunction-sections -funwind-tables -fstack-protector-strong -no-canonical-prefixes -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16 -mthumb -Wa,--noexecstack -Wformat -Werror=format-security  -O0 -fPIC"

# --disable-cli : 关闭命令行
# 其他和ffmpeg一样
./configure \
--prefix=$PREFIX \
--disable-cli \
--enable-static \
--enable-pic \
--host=arm-linux \
--cross-prefix=$TOOLCHAIN/bin/arm-linux-androideabi- \
--sysroot=$NDK_ROOT/platforms/android-17/arch-arm \
--extra-cflags="$CFLAGS"

make clean
make install
```
编译:
```
chmod +x  build.sh
./build.sh
```
将编译产物打包传给windows:
```
cd android/
zip -r x264.zip *
sz x264.zip
```

### AS中集成x264
头文件和库文件导入,CMakelists.txt修改:
```cmake
cmake_minimum_required(VERSION 3.4.1)
#引入指定目录下的CMakeLists.txt
add_subdirectory(librtmp libx264/include)
#指定头文件查找路径
include_directories(librtmp)
add_library(native-lib SHARED native-lib.cpp)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_SOURCE_DIR}/libx264/libs/${CMAKE_ANDROID_ARCH_ABI}")
target_link_libraries(native-lib rtmp x264 log)
```
测试:
```c++
#include <jni.h>
#include <string>
#include <rtmp.h>
#include <x264.h>

extern "C" JNIEXPORT jstring JNICALL
Java_com_netease_pusher_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    char version[100];
    sprintf(version, "librtmp version: %d", RTMP_LibVersion());
    x264_picture_t *picture = new x264_picture_t;
    return env->NewStringUTF(version);
}
```
### NALU
NALU就是NAL UNIT，nal单元。NAL全称Network Abstract Layer, 即网络抽象层，H.264在网络上传输的结构。一帧图片经过 H.264 编码器之后，就被编码为一个或多个片（slice），而装载着这些片（slice）的载体，就是 NALU 了 。

我们通过x264编码获得一组或者多组   `x264_nal_t`。结合RTMP，我们需要区分的是SPS、PPS、关键帧与普通帧：
```c++
enum nal_unit_type_e
{
    NAL_UNKNOWN     = 0,
    NAL_SLICE       = 1,
    NAL_SLICE_DPA   = 2,
    NAL_SLICE_DPB   = 3,
    NAL_SLICE_DPC   = 4,
    NAL_SLICE_IDR   = 5,    /* ref_idc != 0 */ 		//关键帧片
    NAL_SEI         = 6,    /* ref_idc == 0 */
    NAL_SPS         = 7,					 	  //sps片
    NAL_PPS         = 8,						  //pps片
    NAL_AUD         = 9,
    NAL_FILLER      = 12,
    /* ref_idc == 0 for 6,9,10,11,12 */
};
```

### IDR
一段h264视频由N组GOP（group of picture）组成，GOP指的就是画面组，一个GOP是一组连续的画面 。之前的学习中我们知道I帧能够独立解码，而P、B需要参考其他帧。
属于I帧的子集，有一种特殊的I帧，被称之为IDR帧，IDR帧的作用为即时刷新。
![GOP](图片\GOP.png)
上面的这张图片描述的是2组GOP。其他I帧与IDR帧的区别就在于：刷新。当解码器解码帧5的时候，可以跨过帧4参考到帧3，普通I帧不会导致解码器的解码信息数据刷新。而IDR帧则会刷新解码需要的SPS、PPS数据，所以帧8不可能跨帧7参考解码。

### H.264数据   
往RTMP包中填充的是H.264数据，但不是直接将x264编码出来的数据填充进去。
一段包含了N个图像的H.264裸数据，每个NAL之间由：
​	00 00 00 01 或者 00 00 01
进行分割。在分割符之后的第一个字节，就是表示这个nal的类型。

 - 0x67：sps  
 - 0x68：pps  
 - 0x65：IDR

即为上面的 

```c++
NAL_SLICE_IDR   	0x65 & 0x1f = 5  
NAL_SPS       		0x67 & 0x1f = 7,					 	
NAL_PPS       		0x68 & 0x1f = 8,
```

在将数据加入RTMPPacket的时候是需要去除分割符的。
![h264](图片\h264.png)

[1]: http://rtmpdump.mplayerhq.hu/
[2]: http://rtmpdump.mplayerhq.hu/download
[3]: https://www.videolan.org/developers/x264.html