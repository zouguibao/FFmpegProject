//
// Created by zouguibao on 2020-04-22.
//

#ifndef FFMPEGPROJECT_VIDEOSTATUS_H
#define FFMPEGPROJECT_VIDEOSTATUS_H

class VideoStatus {
public:
    bool exit;
    bool pause;
    bool load;
    bool seek;

public:
    VideoStatus();

    ~VideoStatus();
};

#endif //FFMPEGPROJECT_VIDEOSTATUS_H
