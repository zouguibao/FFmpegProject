//
// Created by zouguibao on 2020-04-22.
//

#include "VAudioChannel.h"

VAudioChannel::VAudioChannel(int id, AVRational base) {
    channelId = id;
    time_base = base;
}

VAudioChannel::VAudioChannel(int id, AVRational base, int f) {
    channelId = id;
    time_base = base;
    fps = f;
}