//
// Created by zouguibao on 2020-04-19.
//

#ifndef FFMPEGPROJECT_FFPLAY_H
#define FFMPEGPROJECT_FFPLAY_H

//#include "../../../../../../android-ndk-r16b/sysroot/usr/include/jni.h"
#include <jni.h>
// 播放
void playVideo(JNIEnv *env,jobject instance,jstring videoPath, jobject surface);
// 获取视频时长
jlong getVideoDuration(JNIEnv *env,jobject instance,jstring videoPath);

// 暂停播放
void pauseVideo();

// 停止播放
void stopVideo();

// 获取当前播放时长
jint getCurrentTime();

// 获取视频时长
jlong getDuration();

// 视频解码
jstring avcodecinfo(JNIEnv *env,jobject instance);
void playVideoInThread(JNIEnv *env,jobject instance,jstring videoPath, jobject surface);
#endif //FFMPEGPROJECT_FFPLAY_H
