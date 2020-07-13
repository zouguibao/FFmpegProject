//
// Created by zouguibao on 2020-04-22.
//

#ifndef FFMPEGPROJECT_VFFMPEG_H
#define FFMPEGPROJECT_VFFMPEG_H

#include "pthread.h"
#include "VideoBasePlayer.h"
#include "VideoJavaCall.h"
#include "VAudio.h"
#include "VVideo.h"
#include "VideoStatus.h"
#include "VAudioChannel.h"

extern "C" {
#include <libavformat/avformat.h>
};

class VFFmpeg{
public:
    const char *urlPath = NULL;
    VideoJavaCall *javaCall = NULL;
    pthread_t decodThread;
    AVFormatContext *pFormatCtx = NULL;
    int duration;
    int now_time;
    VAudio *vAudio = NULL;
    VVideo *vVideo = NULL;
    VideoStatus *videoStatus = NULL;
    bool exit = false;
    bool exitByUser = false;
    int mimeType = 1;
    bool isavi = false;
    bool isOnlyMusic = false;
    std::deque<VAudioChannel*> audiochannels;
    std::deque<VAudioChannel*> videochannels;

    pthread_mutex_t init_mutex;
    pthread_mutex_t seek_mutex;
public:
    VFFmpeg(VideoJavaCall *call,const char *urlPath,bool onlymusic);
    ~VFFmpeg();
    int preparedFFmeg();
    int decodeFFmpeg();
    int start();
    int seek(int64_t sec);
    int getDuration();
    int getAvCodecContext(AVCodecParameters *parameters,VideoBasePlayer *player);
    void release();
    void pause();
    void resume();
    int getMimeType(const char* codecName);
    void setAudioChannel(int id);
    void setVideoChannel(int id);
    int getAudioChannels();
    int getVideoWidth();
    int getVideoHeight();
};
#endif //FFMPEGPROJECT_VFFMPEG_H
