//
// Created by zouguibao on 2020-04-22.
//

#include "VVideo.h"

VVideo::VVideo(VideoJavaCall *javaCall1, VAudio *audio1, VideoStatus *playStatus) {
    streamIndex = -1;
    clock = 0;
    javaCall = javaCall1;
    audio = audio1;
    videoStatus = playStatus;
    queue = new VideoFrameQueue(videoStatus);
}

void VVideo::release() {
    if (videoStatus != NULL) {
        videoStatus->exit = true;
    }
    if (queue != NULL) {
        queue->noticeThread();
    }

    int count = 0;
    while (!isExit || !isExit2) {
        LOGE("等待渲染线程结束...%d", count);

        if (count > 1000) {
            isExit = true;
            isExit2 = true;
        }
        count++;
        av_usleep(1000 * 10);
    }
    if (queue != NULL) {
        queue->release();
        delete (queue);
        queue = NULL;
    }
    if (javaCall != NULL) {
        javaCall = NULL;
    }
    if (audio != NULL) {
        audio = NULL;
    }
    if (avCodecContext != NULL) {
        avcodec_close(avCodecContext);
        avcodec_free_context(&avCodecContext);
        avCodecContext = NULL;
    }
    if (videoStatus != NULL) {
        videoStatus = NULL;
    }
}

void *decodeVideoT(void *data) {
    VVideo *vVideo = (VVideo *) data;
    vVideo->decodVideo();
    pthread_exit(&vVideo->videoThread);
}

void *codecFrame(void *data) {
    VVideo *vVideo = (VVideo *) data;
    while (!vVideo->videoStatus->exit) {
        if (vVideo->videoStatus->seek) {
            continue;
        }
        vVideo->isExit2 = false;
        if (vVideo->queue->getAvFrameSize() > 20) {
            continue;
        }

        if (vVideo->codecType == 1) {
            if (vVideo->queue->getAvPacketSize() == 0)//加载
            {
                if (!vVideo->videoStatus->load) {
                    vVideo->javaCall->onLoad(WL_THREAD_CHILD, true);
                    vVideo->videoStatus->load = true;
                }
                continue;
            } else {
                if (vVideo->videoStatus->load) {
                    vVideo->javaCall->onLoad(WL_THREAD_CHILD, false);
                    vVideo->videoStatus->load = false;
                }
            }
        }

        AVPacket *packet = av_packet_alloc();
        if (vVideo->queue->getAvPacket(packet) != 0) {
            av_packet_free(&packet);
            av_free(packet);
            packet = NULL;
            continue;
        }

        int ret = avcodec_send_packet(vVideo->avCodecContext, packet);
        if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
            av_packet_free(&packet);
            av_free(packet);
            packet = NULL;
            continue;
        }
        AVFrame *frame = av_frame_alloc();
        ret = avcodec_receive_frame(vVideo->avCodecContext, frame);
        if (ret < 0 && ret != AVERROR_EOF) {
            av_frame_free(&frame);
            av_free(frame);
            frame = NULL;
            av_packet_free(&packet);
            av_free(packet);
            packet = NULL;
            continue;
        }
        vVideo->queue->putAvframe(frame);
        av_packet_free(&packet);
        av_free(packet);
        packet = NULL;
    }
    vVideo->isExit2 = true;
    pthread_exit(&vVideo->decFrame);
}

void VVideo::playVideo(int type) {
    codecType = type;
    if (codecType == 0) {
        pthread_create(&decFrame, NULL, codecFrame, this);
    }
    pthread_create(&videoThread, NULL, decodeVideoT, this);
}

void VVideo::decodVideo() {
    while (!videoStatus->exit) {
        isExit = false;
        if (videoStatus->pause)//暂停
        {
            continue;
        }
        if (videoStatus->seek) {
            javaCall->onLoad(WL_THREAD_CHILD, true);
            videoStatus->load = true;
            continue;
        }

        if (queue->getAvPacketSize() == 0)//加载
        {
            if (!videoStatus->load) {
                javaCall->onLoad(WL_THREAD_CHILD, true);
                videoStatus->load = true;
            }
            continue;
        } else {
            if (videoStatus->load) {
                javaCall->onLoad(WL_THREAD_CHILD, false);
                videoStatus->load = false;
            }
        }

        if (codecType == 1) {
            AVPacket *packet = av_packet_alloc();
            if (queue->getAvPacket(packet) != 0) {
                av_free(packet->data);
                av_free(packet->buf);
                av_free(packet->side_data);
                packet = NULL;
                continue;
            }

            double time = packet->pts * av_q2d(time_base);

            LOGE("video clock is %f", time);
            LOGE("audio clock is %f", audio->clock);

            if (time < 0) {
                time = packet->dts * av_q2d(time_base);
            }
            if (time < clock) {
                time = clock;
            }
            clock = time;
            double diff = 0;

            if (audio != NULL) {
                diff = audio->clock - clock;
            }
            playcount++;
            if (playcount > 500) {
                playcount = 0;
            }

            if (diff >= 0.5) {
                if (frameratebig) {
                    if (playcount % 3 == 0 && packet->flags != AV_PKT_FLAG_KEY) {
                        av_free(packet->data);
                        av_free(packet->buf);
                        av_free(packet->side_data);
                        packet = NULL;
                        continue;
                    }
                } else {
                    av_free(packet->data);
                    av_free(packet->buf);
                    av_free(packet->side_data);
                    packet = NULL;
                    continue;
                }
            }

            delayTime = getDelayTime(diff);
            LOGE("delay time %f diff is %f", delayTime, diff);
            av_usleep(delayTime * 1000);
            javaCall->onVideoInfo(WL_THREAD_CHILD, clock, duration);
            javaCall->onDecMediacodec(WL_THREAD_CHILD, packet->size, packet->data, 0);
            av_free(packet->data);
            av_free(packet->buf);
            av_free(packet->side_data);
            packet = NULL;
        } else if (codecType == 0) {
            AVFrame *frame = av_frame_alloc();
            if (queue->getAvframe(frame) != 0) {
                av_frame_free(&frame);
                av_free(frame);
                frame = NULL;
                continue;
            }
            if ((framePts = av_frame_get_best_effort_timestamp(frame)) == AV_NOPTS_VALUE) {
                framePts = 0;
            }
            framePts *= av_q2d(time_base);
            clock = synchronize(frame, framePts);
            double diff = 0;
            if (audio != NULL) {
                diff = audio->clock - clock;
            }
            delayTime = getDelayTime(diff);
            LOGE("delay time %f diff is %f", delayTime, diff);
//            if(diff >= 0.8)
//            {
//                av_frame_free(&frame);
//                av_free(frame);
//                frame = NULL;
//                continue;
//            }

            playcount++;
            if (playcount > 500) {
                playcount = 0;
            }
            if (diff >= 0.5) {
                if (frameratebig) {
                    if (playcount % 3 == 0) {
                        av_frame_free(&frame);
                        av_free(frame);
                        frame = NULL;
                        queue->clearToKeyFrame();
                        continue;
                    }
                } else {
                    av_frame_free(&frame);
                    av_free(frame);
                    frame = NULL;
                    queue->clearToKeyFrame();
                    continue;
                }
            }

            av_usleep(delayTime * 1000);
            javaCall->onVideoInfo(WL_THREAD_CHILD, clock, duration);
            javaCall->onGlRenderYuv(WL_THREAD_CHILD, frame->linesize[0], frame->height,
                                    frame->data[0], frame->data[1], frame->data[2]);
            av_frame_free(&frame);
            av_free(frame);
            frame = NULL;
        }
    }
    isExit = true;
}

VVideo::~VVideo() {
    LOGE("video s释放完");
}

double VVideo::synchronize(AVFrame *srcFrame, double pts) {
    double frame_delay;

    if (pts != 0)
        video_clock = pts; // Get pts,then set video clock to it
    else
        pts = video_clock; // Don't get pts,set it to video clock

    frame_delay = av_q2d(time_base);
    frame_delay += srcFrame->repeat_pict * (frame_delay * 0.5);

    video_clock += frame_delay;

    return pts;
}

double VVideo::getDelayTime(double diff) {
    LOGE("audio video diff is %f", diff);
    if (diff > 0.003) {
        delayTime = delayTime / 3 * 2;
        if (delayTime < rate / 2) {
            delayTime = rate / 3 * 2;
        } else if (delayTime > rate * 2) {
            delayTime = rate * 2;
        }

    } else if (diff < -0.003) {
        delayTime = delayTime * 3 / 2;
        if (delayTime < rate / 2) {
            delayTime = rate / 3 * 2;
        } else if (delayTime > rate * 2) {
            delayTime = rate * 2;
        }
    } else if (diff == 0) {
        delayTime = rate;
    }
    if (diff > 1.0) {
        delayTime = 0;
    }
    if (diff < -1.0) {
        delayTime = rate * 2;
    }
    if (fabs(diff) > 10) {
        delayTime = rate;
    }
    return delayTime;
}

void VVideo::setClock(int secds) {
    clock = secds;
}