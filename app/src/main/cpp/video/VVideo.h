//
// Created by zouguibao on 2020-04-22.
//
#pragma once
#ifndef FFMPEGPROJECT_VVIDEO_H
#define FFMPEGPROJECT_VVIDEO_H

#include "VideoBasePlayer.h"
#include "VideoFrameQueue.h"
#include "VideoJavaCall.h"
#include "VAudio.h"

extern "C" {
#include <libavutil/time.h>
};

class VVideo : public VideoBasePlayer {
public:
    VideoFrameQueue *queue = NULL;
    VAudio *audio = NULL;
    VideoStatus *videoStatus = NULL;
    pthread_t videoThread;
    pthread_t decFrame;
    VideoJavaCall *javaCall = NULL;

    double delayTime = 0;
    int rate = 0;
    bool isExit = true;
    bool isExit2 = true;

    int codecType = -1;
    double video_clock = 0;
    double framePts = 0;
    bool frameratebig = false;
    int playcount = -1;
public:
    VVideo(VideoJavaCall *javaCall, VAudio *audio, VideoStatus *playStatus);

    ~VVideo();

    void playVideo(int codecType);

    void decodVideo();

    void release();

    double synchronize(AVFrame *srcFrame, double pts);

    double getDelayTime(double diff);

    void setClock(int secds);

};

#endif //FFMPEGPROJECT_VVIDEO_H
