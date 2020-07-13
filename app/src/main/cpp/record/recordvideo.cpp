//
// Created by zouguibao on 2020-05-02.
//
#include <sys/types.h>
#include "CommondTools.h"
#include "WAVTools.h"
extern "C"{
#include <android/asset_manager_jni.h>
#include <android/asset_manager.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
}
/*
* Class:     com_example_ffmpegproject_record_RecordPreviewScheduler_switchCameraFacing
* Method:    switchCameraFacing
* Signature: ()V
*/
extern "C"
JNIEXPORT void JNICALL Java_com_example_ffmpegproject_record_RecordPreviewScheduler_switchCameraFacing
        (JNIEnv *, jobject){

}

/*
 * Class:     com_example_ffmpegproject_record_RecordPreviewScheduler
 * Method:    prepareEGLContext
 * Signature: (Ljava/lang/Object;III)V
 */
extern "C"
JNIEXPORT void JNICALL Java_com_example_ffmpegproject_record_RecordPreviewScheduler_prepareEGLContext
        (JNIEnv *, jobject, jobject, jint, jint, jint){

}

/*
 * Class:     com_example_ffmpegproject_record_RecordPreviewScheduler
 * Method:    createWindowSurface
 * Signature: (Ljava/lang/Object;)V
 */
extern "C"
JNIEXPORT void JNICALL Java_com_example_ffmpegproject_record_RecordPreviewScheduler_createWindowSurface
        (JNIEnv *, jobject, jobject){

}

/*
 * Class:     com_example_ffmpegproject_record_RecordPreviewScheduler
 * Method:    resetRenderSize
 * Signature: (II)V
 */
extern "C"
JNIEXPORT void JNICALL Java_com_example_ffmpegproject_record_RecordPreviewScheduler_resetRenderSize
        (JNIEnv *, jobject, jint, jint){

}

/*
 * Class:     com_example_ffmpegproject_record_RecordPreviewScheduler
 * Method:    destroyWindowSurface
 * Signature: ()V
 */
extern "C"
JNIEXPORT void JNICALL Java_com_example_ffmpegproject_record_RecordPreviewScheduler_destroyWindowSurface
        (JNIEnv *, jobject){

}

/*
 * Class:     com_example_ffmpegproject_record_RecordPreviewScheduler
 * Method:    destroyEGLContext
 * Signature: ()V
 */
extern "C"
JNIEXPORT void JNICALL Java_com_example_ffmpegproject_record_RecordPreviewScheduler_destroyEGLContext
        (JNIEnv *, jobject){

}

/*
 * Class:     com_example_ffmpegproject_record_RecordPreviewScheduler
 * Method:    notifyFrameAvailable
 * Signature: ()V
 */
extern "C"
JNIEXPORT void JNICALL Java_com_example_ffmpegproject_record_RecordPreviewScheduler_notifyFrameAvailable
        (JNIEnv *, jobject){

}