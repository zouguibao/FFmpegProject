//
// Created by zouguibao on 2020-04-22.
//

#ifndef FFMPEGPROJECT_VIDEOBASEPLAYER_H
#define FFMPEGPROJECT_VIDEOBASEPLAYER_H

extern "C" {
#include <libavcodec/avcodec.h>
};

class VideoBasePlayer{
public:
    int streamIndex;
    int duration;
    double clock = 0;
    double now_time = 0;
    AVCodecContext *avCodecContext = NULL;
    AVRational time_base;
public:
    VideoBasePlayer();
    ~VideoBasePlayer();
};


#endif //FFMPEGPROJECT_VIDEOBASEPLAYER_H
