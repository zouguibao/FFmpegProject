//
// Created by zouguibao on 2020-04-15.
//


#include <jni.h>
#include <string>
#include <stdio.h>
#include <unistd.h>


extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/mem.h>
#include <libavutil/error.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>
#include "ffplay.h"

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "zouguibao", __VA_ARGS__)
//int64_t duration = 0;
//double currentTime;
//int pausePlay = 0;
//int stopPlay = 0;
extern "C"
jstring
Java_com_example_ffmpegproject_MainActivity_avcodecinfo(JNIEnv *env, jobject instance) {

    return avcodecinfo(env, instance);
}

extern "C"
jstring
Java_com_example_ffmpegproject_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}


extern "C"
void
Java_com_example_ffmpegproject_MainActivity_ffPause(JNIEnv *env, jobject instance) {
    pauseVideo();
}
extern "C"
void
Java_com_example_ffmpegproject_MainActivity_ffStop(JNIEnv *env, jobject instance) {
    stopVideo();
}
extern "C"
jlong
Java_com_example_ffmpegproject_MainActivity_getDuration(JNIEnv *env, jobject instance) {
    return getDuration();
}

extern "C"
jint
Java_com_example_ffmpegproject_MainActivity_getCurrentTime(JNIEnv *env, jobject instance) {
    return getCurrentTime();
}

extern "C"
jlong
Java_com_example_ffmpegproject_MainActivity_getVideoDuration(JNIEnv *env, jobject instance,
                                                             jstring videoPath) {
    return getVideoDuration(env, instance, videoPath);
}

extern "C"
void
Java_com_example_ffmpegproject_MainActivity_ffplay(JNIEnv *env, jobject instance,
                                                   jstring videoPath, jobject surface) {
//    playVideoInThread(env,instance,videoPath,surface);
    playVideo(env, instance, videoPath, surface);
}

}

