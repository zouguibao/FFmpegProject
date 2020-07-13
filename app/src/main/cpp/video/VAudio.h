//
// Created by zouguibao on 2020-04-22.
//
#pragma once
#ifndef FFMPEGPROJECT_VAUDIO_H
#define FFMPEGPROJECT_VAUDIO_H

#include "VideoBasePlayer.h"
#include "VideoFrameQueue.h"
#include "VideoStatus.h"
#include "VideoJavaCall.h"

extern "C"{
#include "libswresample/swresample.h"
#include "libavutil/time.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
};

class WlOpenSLES;
class VAudio : public VideoBasePlayer{
public:
    VideoFrameQueue *queue = NULL;
    VideoStatus *videoStatus = NULL;
    VideoJavaCall *javaCall = NULL;
    pthread_t audioThread;

    int ret = 0;//函数返回结果
    int64_t dst_layout = 0;//重采样为立体声
    int dst_nb_samples = 0;//计算转换后的sample个数 a * b / c
    int nb = 0;//转换 返回值转换后的sample个数
    uint8_t *out_buffer = NULL;//buffer 内存区域
    int out_channels = 0;//输出声道数
    int data_size = 0;//buffer大小

    enum AVSampleFormat dst_format;

    void *buffer = NULL;
    int pcmsize = 0;
    int sample_rate = 44100;
    bool isExit = false;
    bool isVideo = false;

    bool isReadPacketFinish = true;
    AVPacket *packet;

    // 引擎接口
    SLObjectItf engineObject = NULL;
    SLEngineItf engineEngine = NULL;

    //混音器
    SLObjectItf  outputMixObject = NULL;
    SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;
    SLEnvironmentalReverbSettings reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

    //pcm
    SLObjectItf pcmPlayerObject = NULL;
    SLPlayItf pcmPlayerPlay = NULL;
    SLVolumeItf pcmPlayerVolume = NULL;

    // 缓冲器队列接口
    SLAndroidSimpleBufferQueueItf pcmBufferQueue = NULL;

public:
    VAudio(VideoStatus *playStatus,VideoJavaCall *javaCall1);
    ~VAudio();
    void setVideo(bool video);

    void playAudio();
    int getPcmData(void **pcm);
    int initOpenSL();
    void pause();
    void resume();
    void realease();
    SLuint32 getSLSampleRate();
    void setClock(int secds);
};


#endif //FFMPEGPROJECT_VAUDIO_H
