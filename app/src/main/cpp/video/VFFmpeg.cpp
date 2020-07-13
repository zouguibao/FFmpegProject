//
// Created by zouguibao on 2020-04-22.
//

#include "VFFmpeg.h"


VFFmpeg::VFFmpeg(VideoJavaCall *call, const char *url, bool onlymusic) {
    pthread_mutex_init(&init_mutex, NULL);
    pthread_mutex_init(&seek_mutex, NULL);
    exitByUser = false;
    isOnlyMusic = onlymusic;
    javaCall = call;
    urlPath = url;
    videoStatus = new VideoStatus();

}

VFFmpeg::~VFFmpeg() {
    pthread_mutex_destroy(&init_mutex);
    LOGE("~WlFFmpeg() 释放了");
}

void *decodeThread(void *data) {
    VFFmpeg *wlFFmpeg = (VFFmpeg *) data;
    wlFFmpeg->decodeFFmpeg();
    pthread_exit(&wlFFmpeg->decodThread);
}

int VFFmpeg::preparedFFmeg() {
    pthread_create(&decodThread, NULL, decodeThread, this);
    return 0;
}

int avformat_intterupt_cb(void *ctx) {
    VFFmpeg *vfFmpeg = (VFFmpeg *) ctx;
    if (vfFmpeg->videoStatus->exit) {
        LOGE("avformat_interrupt_cb return 1");
        return AVERROR_EOF;
    }
    LOGE("avformat_interrupt_cb return 0");
    return 0;
}

int VFFmpeg::getAvCodecContext(AVCodecParameters *parameters, VideoBasePlayer *player) {
    AVCodec *avCodec = avcodec_find_decoder(parameters->codec_id);
    if (!avCodec) {
        javaCall->onError(WL_THREAD_CHILD, 3, "get avcodec fail");
        exit = true;
        return 1;
    }
    player->avCodecContext = avcodec_alloc_context3(avCodec);
    if (!player->avCodecContext) {
        javaCall->onError(WL_THREAD_CHILD, 4, "alloc avcodecctx fail");
        exit = true;
        return 1;
    }

    if (avcodec_parameters_to_context(player->avCodecContext, parameters) != 0) {
        javaCall->onError(WL_THREAD_CHILD, 5, "copy avcodecctx fail");
        exit = true;
        return 1;
    }

    if (avcodec_open2(player->avCodecContext, avCodec, 0) != 0) {
        javaCall->onError(WL_THREAD_CHILD, 6, "open avcodecctx fail");
        exit = true;
        return 1;
    }

    return 0;
}

int VFFmpeg::decodeFFmpeg() {
    LOGE("decodeFFmpeg111111111111");
    pthread_mutex_lock(&init_mutex);
    exit = false;
    isavi = false;
    av_register_all();
    avformat_network_init();
    pFormatCtx = avformat_alloc_context();
    if (avformat_open_input(&pFormatCtx, urlPath, NULL, NULL) != 0) {
        LOGE("can not open url:%s", urlPath);
        if (javaCall != NULL) {
            javaCall->onError(WL_THREAD_CHILD, WL_FFMPEG_CAN_NOT_OPEN_URL, "can not open url");
        }
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGE("can not find streams from %s", urlPath);
        if (javaCall != NULL) {
            javaCall->onError(WL_THREAD_CHILD, WL_FFMPEG_CAN_NOT_FIND_STREAMS,
                              "can not find streams from url");
        }
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }

    if (pFormatCtx == NULL) {
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }

    duration = pFormatCtx->duration / 1000000;
    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
        //获取音频
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            LOGE("音频");
            VAudioChannel *vAudioChannel = new VAudioChannel(i, pFormatCtx->streams[i]->time_base);
            audiochannels.push_front(vAudioChannel);
        } else if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (!isOnlyMusic) {
                LOGE("视频");
                int num = pFormatCtx->streams[i]->avg_frame_rate.num;
                int den = pFormatCtx->streams[i]->avg_frame_rate.den;
                if (num != 0 && den != 0) {
                    int fps = pFormatCtx->streams[i]->avg_frame_rate.num /
                              pFormatCtx->streams[i]->avg_frame_rate.den;
                    VAudioChannel *wl = new VAudioChannel(i, pFormatCtx->streams[i]->time_base,
                                                          fps);
                    videochannels.push_front(wl);
                }
            }
        }
    }

    LOGE("decodeFFmpeg111111111111  audiochannel size = %d ",audiochannels.size());
    if (audiochannels.size() > 0) {
        vAudio = new VAudio(videoStatus, javaCall);
        setAudioChannel(0);
        if (vAudio->streamIndex >= 0 && vAudio->streamIndex < pFormatCtx->nb_streams) {
            if (getAvCodecContext(pFormatCtx->streams[vAudio->streamIndex]->codecpar, vAudio) !=
                0) {
                exit = true;
                pthread_mutex_unlock(&init_mutex);
                return 1;
            }
        }
    }
    LOGE("decodeFFmpeg111111111111  videochannel size = %d ",videochannels.size());
    if (videochannels.size() > 0) {
        vVideo = new VVideo(javaCall, vAudio, videoStatus);
        setVideoChannel(0);
        if (vVideo->streamIndex >= 0 && vVideo->streamIndex < pFormatCtx->nb_streams) {
            if (getAvCodecContext(pFormatCtx->streams[vVideo->streamIndex]->codecpar, vVideo) !=
                0) {
                exit = true;
                pthread_mutex_unlock(&init_mutex);
                return 1;
            }
        }
    }


    if (vAudio == NULL && vVideo == NULL) {
        LOGE("audio and video double null");
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return 1;
    }
    if (vAudio != NULL) {
        vAudio->duration = pFormatCtx->duration / 1000000;
        vAudio->sample_rate = vAudio->avCodecContext->sample_rate;
        if (vVideo != NULL) {
            vAudio->setVideo(true);
        }
    }
    if (vVideo != NULL) {

        LOGE("codec name is %s", vVideo->avCodecContext->codec->name);
        LOGE("codec long name is %s", vVideo->avCodecContext->codec->long_name);

        if (!javaCall->isOnlySoft(WL_THREAD_CHILD)) {
            mimeType = getMimeType(vVideo->avCodecContext->codec->name);
        } else {
            mimeType = -1;
        }
        LOGE("mimeType = %d",mimeType);
        if (mimeType != -1) {
            javaCall->onInitMediacodec(WL_THREAD_CHILD, mimeType, vVideo->avCodecContext->width,
                                       vVideo->avCodecContext->height,
                                       vVideo->avCodecContext->extradata_size,
                                       vVideo->avCodecContext->extradata_size,
                                       vVideo->avCodecContext->extradata,
                                       vVideo->avCodecContext->extradata);
        }
        vVideo->duration = pFormatCtx->duration / 1000000;
    }

    LOGE("准备ing");
    javaCall->onParpared(WL_THREAD_CHILD);
    LOGE("准备end");
    exit = true;
    pthread_mutex_unlock(&init_mutex);
    return 0;
}

int VFFmpeg::getDuration() {
    return duration;
}

int VFFmpeg::start() {
    exit = false;
    int count = 0;
    int ret = -1;
    if (vAudio != NULL) {
        vAudio->playAudio();
    }
    if (vVideo != NULL) {
        if (mimeType == -1) {
            vVideo->playVideo(0);
        } else {
            vVideo->playVideo(1);
        }
    }

    AVBitStreamFilterContext *mimType = NULL;
    if (mimeType == 1) {
        mimType = av_bitstream_filter_init("h264_mp4toannexb");
    } else if (mimeType == 2) {
        mimType = av_bitstream_filter_init("hevc_mp4toannexb");
    } else if (mimeType == 3) {
        mimType = av_bitstream_filter_init("h264_mp4toannexb");
    } else if (mimeType == 4) {
        mimType = av_bitstream_filter_init("h264_mp4toannexb");
    }

    while (!videoStatus->exit) {
        exit = false;
        if (videoStatus->pause)//暂停
        {
            av_usleep(1000 * 100);
            continue;
        }
        if (vAudio != NULL && vAudio->queue->getAvPacketSize() > 100) {
//            LOGE("wlAudio 等待..........");
            av_usleep(1000 * 100);
            continue;
        }
        if (vVideo != NULL && vVideo->queue->getAvPacketSize() > 100) {
//            LOGE("wlVideo 等待..........");
            av_usleep(1000 * 100);
            continue;
        }
        AVPacket *packet = av_packet_alloc();
        pthread_mutex_lock(&seek_mutex);
        ret = av_read_frame(pFormatCtx, packet);
        pthread_mutex_unlock(&seek_mutex);
        if (videoStatus->seek) {
            av_packet_free(&packet);
            av_free(packet);
            continue;
        }
        if (ret == 0) {
            if (vAudio != NULL && packet->stream_index == vAudio->streamIndex) {
                count++;
                LOGE("解码第 %d 帧", count);
                vAudio->queue->putAvPacket(packet);
            } else if (vVideo != NULL && packet->stream_index == vVideo->streamIndex) {
                if (mimType != NULL && !isavi) {
                    uint8_t *data;
                    av_bitstream_filter_filter(mimType,
                                               pFormatCtx->streams[vVideo->streamIndex]->codec,
                                               NULL, &data, &packet->size, packet->data,
                                               packet->size, 0);
                    uint8_t *tdata = NULL;
                    tdata = packet->data;
                    packet->data = data;
                    if (tdata != NULL) {
                        av_free(tdata);
                    }
                }
                vVideo->queue->putAvPacket(packet);
            } else {
                av_packet_free(&packet);
                av_free(packet);
                packet = NULL;
            }
        } else {
            av_packet_free(&packet);
            av_free(packet);
            packet = NULL;
            if ((vVideo != NULL && vVideo->queue->getAvFrameSize() == 0) ||
                (vAudio != NULL && vAudio->queue->getAvPacketSize() == 0)) {
                videoStatus->exit = true;
                break;
            }
        }
    }
    if (mimType != NULL) {
        av_bitstream_filter_close(mimType);
    }
    if (!exitByUser && javaCall != NULL) {
        javaCall->onComplete(WL_THREAD_CHILD);
    }
    exit = true;
    return 0;
}

void VFFmpeg::release() {
    videoStatus->exit = true;
    pthread_mutex_lock(&init_mutex);
    LOGE("开始释放 wlffmpeg");
    int sleepCount = 0;
    while (!exit) {
        if (sleepCount > 1000)//十秒钟还没有退出就自动强制退出
        {
            exit = true;
        }
        LOGE("wait ffmpeg  exit %d", sleepCount);

        sleepCount++;
        av_usleep(1000 * 10);//暂停10毫秒
    }
    LOGE("释放audio....................................");

    if (vAudio != NULL) {
        LOGE("释放audio....................................2");

        vAudio->realease();
        delete (vAudio);
        vAudio = NULL;
    }
    LOGE("释放video....................................");

    if (vVideo != NULL) {
        LOGE("释放video....................................2");

        vVideo->release();
        delete (vVideo);
        vVideo = NULL;
    }
    LOGE("释放format...................................");

    if (pFormatCtx != NULL) {
        avformat_close_input(&pFormatCtx);
        avformat_free_context(pFormatCtx);
        pFormatCtx = NULL;
    }
    LOGE("释放javacall.................................");

    if (javaCall != NULL) {
        javaCall = NULL;
    }
    pthread_mutex_unlock(&init_mutex);
}

void VFFmpeg::pause() {
    if (videoStatus != NULL) {
        videoStatus->pause = true;
        if (vAudio != NULL) {
            vAudio->pause();
        }
    }
}

void VFFmpeg::resume() {
    if (videoStatus != NULL) {
        videoStatus->pause = false;
        if (vAudio != NULL) {
            vAudio->resume();
        }
    }
}

int VFFmpeg::getMimeType(const char *codecName) {

    if (strcmp(codecName, "h264") == 0) {
        return 1;
    }
    if (strcmp(codecName, "hevc") == 0) {
        return 2;
    }
    if (strcmp(codecName, "mpeg4") == 0) {
        isavi = true;
        return 3;
    }
    if (strcmp(codecName, "wmv3") == 0) {
        isavi = true;
        return 4;
    }

    return -1;
}

int VFFmpeg::seek(int64_t sec) {
    if (sec >= duration) {
        return -1;
    }
    if (pFormatCtx != NULL) {
        videoStatus->seek = true;
        pthread_mutex_lock(&seek_mutex);
        int64_t rel = sec * AV_TIME_BASE;
        int ret = avformat_seek_file(pFormatCtx, -1, INT64_MIN, rel, INT64_MAX, 0);
        if (vAudio != NULL) {
            vAudio->queue->clearAvpacket();
            vAudio->setClock(0);
        }

        if (vVideo != NULL) {
            vVideo->queue->clearAvFrame();
            vVideo->queue->clearAvpacket();
            vVideo->setClock(0);
        }

        vAudio->clock = 0;
        vAudio->now_time = 0;
        pthread_mutex_unlock(&seek_mutex);
        videoStatus->seek = false;
    }
    return 0;
}

void VFFmpeg::setAudioChannel(int index) {
    if (vAudio != NULL) {
        int channelsize = audiochannels.size();
        if (index < channelsize) {
            for (int i = 0; i < channelsize; i++) {
                if (i == index) {
                    vAudio->time_base = audiochannels.at(i)->time_base;
                    vAudio->streamIndex = audiochannels.at(i)->channelId;
                }
            }
        }
    }
}

void VFFmpeg::setVideoChannel(int id) {
    if (vVideo != NULL) {
        vVideo->streamIndex = videochannels.at(id)->channelId;
        vVideo->time_base = videochannels.at(id)->time_base;
        vVideo->rate = 1000 / videochannels.at(id)->fps;
        if (videochannels.at(id)->fps >= 60) {
            vVideo->frameratebig = true;
        } else {
            vVideo->frameratebig = false;
        }
    }
}

int VFFmpeg::getAudioChannels() {
    return audiochannels.size();
}

int VFFmpeg::getVideoWidth() {
    if (vVideo != NULL && vVideo->avCodecContext != NULL) {
        return vVideo->avCodecContext->width;
    }
    return 0;
}

int VFFmpeg::getVideoHeight() {
    if (vVideo != NULL && vVideo->avCodecContext != NULL) {
        return vVideo->avCodecContext->height;
    }
    return 0;
}