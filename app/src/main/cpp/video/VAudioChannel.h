//
// Created by zouguibao on 2020-04-22.
//

#ifndef FFMPEGPROJECT_VAUDIOCHANNEL_H
#define FFMPEGPROJECT_VAUDIOCHANNEL_H

extern "C" {
#include "libavutil/rational.h"
};

class VAudioChannel {
public:
    int channelId = -1;
    AVRational time_base;
    int fps;
public:
    VAudioChannel(int id, AVRational base);

    VAudioChannel(int id, AVRational base, int fps);
};

#endif //FFMPEGPROJECT_VAUDIOCHANNEL_H
