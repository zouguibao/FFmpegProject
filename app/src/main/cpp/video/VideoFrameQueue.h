//
// Created by zouguibao on 2020-04-22.
//

#ifndef FFMPEGPROJECT_VIDEOFRAMEQUEUE_H
#define FFMPEGPROJECT_VIDEOFRAMEQUEUE_H

#include "VideoStatus.h"
#include "queue"
#include "android/log.h"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "zouguibao", __VA_ARGS__)
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavutil/frame.h"
#include "libavutil/mem.h"
#include "libavutil/imgutils.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
#include "pthread.h"
};



class VideoFrameQueue{
public:
    std::queue<AVPacket*> queuePacket;
    std::queue<AVFrame*> queueFrame;
    pthread_mutex_t mutexFrame;
    pthread_cond_t condFrame;
    pthread_mutex_t mutexPacket;
    pthread_cond_t condPacket;
    VideoStatus *videoStatus = NULL;

public:
    VideoFrameQueue(VideoStatus *videoStatus1);
    ~VideoFrameQueue();

    int putAvPacket(AVPacket *avPacket);
    int getAvPacket(AVPacket *avPacket);
    int clearAvpacket();
    int clearToKeyFrame();

    int putAvframe(AVFrame *avFrame);
    int getAvframe(AVFrame *avFrame);
    int clearAvFrame();

    void release();
    int getAvPacketSize();
    int getAvFrameSize();

    int noticeThread();
};

#endif //FFMPEGPROJECT_VIDEOFRAMEQUEUE_H
