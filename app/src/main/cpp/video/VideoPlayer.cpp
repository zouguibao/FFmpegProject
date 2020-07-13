//
// Created by zouguibao on 2020-04-21.
//

#include <jni.h>
#include <stddef.h>
#include "VFFmpeg.h"
#include "VideoJavaCall.h"

_JavaVM *javaVM = NULL;
VideoJavaCall *javaCall = NULL;
VFFmpeg *vfFmpeg = NULL;

extern "C"
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    jint result = -1;
    javaVM = vm;
    JNIEnv *env;
    LOGE("JNI_OnLoad.............");
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        LOGE("GetEnv failed!");
        return result;
    }

    return JNI_VERSION_1_4;
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_ffmpegproject_video_VideoPlayer_hello(JNIEnv *env, jobject instance){
    LOGE("Hello NDK C++");
    return env->NewStringUTF("Hello NDK C++");
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_ffmpegproject_video_VideoPlayer_prepared(JNIEnv *env, jobject instance,
                                                          jstring url_, jboolean isOnlyMusic) {
    const char *url = env->GetStringUTFChars(url_, 0);
    LOGE("JNI_prepared............. %s",url);
    if (javaCall == NULL) {
        javaCall = new VideoJavaCall(javaVM, env, instance);
    }

    if (vfFmpeg == NULL) {
        LOGE("JNI_prepared.............");
        vfFmpeg = new VFFmpeg(javaCall, url, isOnlyMusic);
        javaCall->onLoad(WL_THREAD_MAIN, true);
        vfFmpeg->preparedFFmeg();
    }

}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_ffmpegproject_video_VideoPlayer_start(JNIEnv *env, jobject instance) {
    if (vfFmpeg != NULL) {
        vfFmpeg->start();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_ffmpegproject_video_VideoPlayer_stop(JNIEnv *env, jobject instance,bool exit) {
    if (vfFmpeg != NULL) {
        vfFmpeg->exitByUser = true;
        vfFmpeg->release();
        vfFmpeg = NULL;
        if (javaCall != NULL) {
            javaCall->release();
            javaCall = NULL;
        }
        if (!exit) {
            jclass jlz = env->GetObjectClass(instance);
            jmethodID jmid_stop = env->GetMethodID(jlz, "onStopComplete", "()V");
            env->CallVoidMethod(instance, jmid_stop);
        }
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_ffmpegproject_video_VideoPlayer_pause(JNIEnv *env, jobject instance) {
    if (vfFmpeg != NULL) {
        vfFmpeg->pause();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_ffmpegproject_video_VideoPlayer_resume(JNIEnv *env, jobject instance) {
    if (vfFmpeg != NULL) {
        vfFmpeg->resume();
    }
}


extern "C"
JNIEXPORT void JNICALL
Java_com_example_ffmpegproject_video_VideoPlayer_seek(JNIEnv *env, jobject instance, jint secds) {
    if (vfFmpeg != NULL) {
        vfFmpeg->seek(secds);
    }
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_ffmpegproject_video_VideoPlayer_getDuration(JNIEnv *env, jobject instance) {
    if (vfFmpeg != NULL) {
        vfFmpeg->getDuration();
    }
    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_ffmpegproject_video_VideoPlayer_getAudioChannels(JNIEnv *env, jobject instance) {
    if (vfFmpeg != NULL) {
        vfFmpeg->getAudioChannels();
    }
    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_ffmpegproject_video_VideoPlayer_getVideoWidth(JNIEnv *env, jobject instance) {
    if (vfFmpeg != NULL) {
        vfFmpeg->getVideoWidth();
    }
    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_ffmpegproject_video_VideoPlayer_getVideoHeight(JNIEnv *env, jobject instance) {
    if (vfFmpeg != NULL) {
        vfFmpeg->getVideoHeight();
    }
    return 0;
}


extern "C"
JNIEXPORT void JNICALL
Java_com_example_ffmpegproject_video_VideoPlayer_setAudioChannels(JNIEnv *env, jobject instance, jint index) {

    if (vfFmpeg != NULL) {
        vfFmpeg->setAudioChannel(index);
    }

}
