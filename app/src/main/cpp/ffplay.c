//
// Created by zouguibao on 2020-04-19.
//
#include <jni.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "malloc.h"
#include "stdlib.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/mem.h>
#include <libavutil/error.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>
#include "pthread.h"

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "zouguibao", __VA_ARGS__)
int64_t duration = 0;
double currentTime;
int pausePlay = 0;
int stopPlay = 0;
pthread_t ntid;
int res;
int currentFrame;
struct PlayParam {
    JNIEnv *env;
    jobject instance;
    jstring videoPath;
    jobject surface;
};

void stopVideo() {
    stopPlay = 1;
}

void pauseVideo() {
    pausePlay = 1;
}

jint getCurrentTime() {
    return currentTime * 1000;
}

jlong getDuration() {
    return duration / 1000;
}

void playVideo(JNIEnv *env, jobject instance,
               jstring videoPath, jobject surface) {
    LOGE("playVideo......111111111111111");
    const char *input = (*env)->GetStringUTFChars(env, videoPath, NULL);
    LOGE("playVideo......2222222222222222222");
    if (input == NULL) {
        LOGE("字符串转换失败......");
        return;
    }
    // 获取回调的java类
    jclass jcls = (*env)->FindClass(env, "com/example/ffmpegproject/OnVideoPlayListener");
    // 注册FFmpeg所有编解码器以及相关协议
    av_register_all();
    avformat_network_init();
    // 分配结构体
    AVFormatContext *formatContext = avformat_alloc_context();

    // 打开视频数据源 需要本地权限
    int open_state = avformat_open_input(&formatContext, input, NULL, NULL);
    if (open_state < 0) {
        char errbuf[128];
        if (av_strerror(open_state, errbuf, sizeof(errbuf) == 0)) {
            LOGE("打开视频输入流信息失败，失败原因：%s", errbuf);
        }
        return;
    }

    // 为分配的AVFormatContenxt结构体中填充数据
    if (avformat_find_stream_info(formatContext, NULL) < 0) {
        LOGE("读取视频输入流信息失败");
        return;
    }

    if (formatContext->duration != AV_NOPTS_VALUE) {
        duration = formatContext->duration;
    }

    if (jcls != NULL) {
        jmethodID jmethodId = (*env)->GetMethodID(env, jcls, "onPrepared", "()V");
        (*env)->CallVoidMethod(env, instance, jmethodId, NULL);
    }

    // 记录视频流所在数组下标
    int video_stream_index = -1;
    LOGE("当前视频数据，包含的数据流数量：%d", formatContext->nb_streams);
    // 找到视频流AVFormatContenxt结构体中的nb_streams字段存储的数据就是当前视频文件中所包含的总数据流数量
    // 视频流 音频流 字幕流
    for (int i = 0; i < formatContext->nb_streams; i++) {
        // 如果是数据流，则编码格式为AVMEDIA_TYPE_VIDEO - 视频流
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;//记录视频流下标
            break;
        }
    }
    LOGE("playVideo......3333333333333");
    if (video_stream_index == -1) {
        LOGE("没有找到视频流");
        return;
    }

    //通过编解码器的id - codec_id 获取对应视频流的解码器
    AVCodecParameters *codecParameters = formatContext->streams[video_stream_index]->codecpar;
    AVCodec *videoDecoder = avcodec_find_decoder(codecParameters->codec_id);
    LOGE("playVideo......44444444444444444");
    if (videoDecoder == NULL) {
        LOGE("没有找到视频流的解码器");
        return;
    }

    // 通过解码器分配（并用 默认值 初始化）一个解码器context
    AVCodecContext *codecContext = avcodec_alloc_context3(videoDecoder);

    if (codecContext == NULL) {
        LOGE("分配解码器上下文失败");
        return;
    }

    //根据指定的编码器的值填充编码器的上下文
    if (avcodec_parameters_to_context(codecContext, codecParameters) < 0) {
        LOGE("填充解码器上下文失败");
        return;
    }
    // 通过所给的编解码器初始化解码器上下文
    if (avcodec_open2(codecContext, videoDecoder, NULL)) {
        LOGE("初始化解码器上下文失败");
        return;
    }

    // 分配存储压缩数据的结构体对象ACPacket
    // 如果是视频流，ACPacket会包含一帧的压缩数据，如果是音频流则可能包含多帧压缩数据
    enum AVPixelFormat dstFormat = AV_PIX_FMT_RGBA;
    AVPacket *avPacket = av_packet_alloc();
    // 分配解码后的每一帧数据信息的结构体（指针）
    AVFrame *avFrame = av_frame_alloc();
    // 分配最终显示出来的目标帧信息的结构体（指针）
    AVFrame *outAvFrame = av_frame_alloc();

    // Determine required buffer size and allocate buffer
    // buffer中数据就是用于渲染的,且格式为RGBA
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, codecContext->width,
                                            codecContext->height, 1);
    uint8_t *out_buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

    av_image_fill_arrays(
            outAvFrame->data,
            outAvFrame->linesize,
            out_buffer,
            dstFormat,
            codecContext->width,
            codecContext->height,
            1
    );

    // 初始化SwsContext
    struct SwsContext *swsContext = sws_getContext(
            codecContext->width, // 原图宽
            codecContext->height, // 原图高
            codecContext->pix_fmt, // 原图format
            codecContext->width, // 目标宽
            codecContext->height, // 目标高
            dstFormat,
            SWS_BICUBIC,
            NULL,
            NULL,
            NULL
    );

    if (swsContext == NULL) {
        LOGE("swsContext上下文初始化失败");
        return;
    }

    // Android 原生绘制工具
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);

    // 定义绘图缓冲区
    ANativeWindow_Buffer outBuffer;

    // 通过设置宽高限制缓冲区中的像素数量，而非屏幕的物理显示尺寸
    // 如果缓冲区与物理屏幕的显示尺寸不相符，则实现显示可能会拉伸，或者被压缩
    ANativeWindow_setBuffersGeometry(nativeWindow, codecContext->width, codecContext->height,
                                     WINDOW_FORMAT_RGBA_8888);
    pausePlay = 0;
    stopPlay = 0;
    int nextIndex = 0;
    LOGE("maxStreams =  %d",formatContext->max_streams);
    // 循环读取数据流的下一帧
    while (av_read_frame(formatContext, avPacket) == 0) {
//        LOGE("avPacket->stream_index = %d", avPacket->stream_index);
//        LOGE("video_stream_index = %d", video_stream_index);
        // 停止播放
        if (stopPlay == 1 || pausePlay == 1) {
            av_packet_unref(avPacket);
            break;
        }
        LOGE("playVideo......555555555555555");
        if (avPacket->stream_index == video_stream_index) {
//            if(pausePlay = 0 && currentFrame > 0 && currentFrame != nextIndex){
//                nextIndex++;
//                av_packet_unref(avPacket);
//                continue;
//            }
//            currentFrame++;
            // 将原始数据发送到解码器
            int sendPacketState = avcodec_send_packet(codecContext, avPacket);
            if (sendPacketState == 0) {
                int receiveFrameState = avcodec_receive_frame(codecContext, avFrame);
                if (receiveFrameState == 0) {
                    // 锁定窗口绘图界面
                    ANativeWindow_lock(nativeWindow, &outBuffer, NULL);
                    // 对输出图像进行色彩，分辨率缩放，滤波处理
                    sws_scale(swsContext, (const uint8_t *const *) avFrame->data, avFrame->linesize,
                              0, avFrame->height, outAvFrame->data, outAvFrame->linesize);

                    uint8_t *dst = (uint8_t *) outBuffer.bits;
                    // 解码后的像素数据首地址
                    // 这里使用的是RGBA格式，所以解码图像数据只保存在data[0]中，但是如果是YUV，则就会有data[0]
                    uint8_t *src = outAvFrame->data[0];
                    // 获取一行字节数
                    int oneLineByte = outBuffer.stride * 4;
                    // 复制一行内存的实际数量
                    int srcStride = outAvFrame->linesize[0];

                    for (int i = 0; i < codecContext->height; i++) {
                        memcpy(dst + i * oneLineByte, src + i * srcStride, srcStride);
                    }

                    // 解锁
                    ANativeWindow_unlockAndPost(nativeWindow);
                    // 当前播放时间
                    double playCurrentTime = avPacket->pts *
                                             av_q2d(formatContext->streams[video_stream_index]->time_base);
                    currentTime = (int) (playCurrentTime * 10) / 10;
                    if (jcls != NULL) {
                        jmethodID jmethodId = (*env)->GetMethodID(env, jcls,
                                                                  "onUpdateCurrentPosition",
                                                                  "()V");
                        (*env)->CallVoidMethod(env, instance, jmethodId);
                    }
                    LOGE("当前播放时间 currentTime = %lf", currentTime);
                    // 进行短暂休眠，如果休眠时间太长会导致播放的每帧画面有延迟感，如果太短则会有加速播放的感觉
                    // 一般一秒60帧 16毫秒一帧的时间进行休眠
                    usleep(1000 * 16);
                } else if (receiveFrameState == AVERROR(EAGAIN)) {
                    LOGE("从解码器接收数据失败：AVERROR(EAGAIN)");
                } else if (receiveFrameState == AVERROR_EOF) {
                    LOGE("从解码器接收数据失败：AVERROR_EOF");
                } else if (receiveFrameState == AVERROR(EINVAL)) {
                    LOGE("从解码器接收数据失败：AVERROR(EINVAL)");
                } else {
                    LOGE("从解码器接收数据失败：未知");
                }
            } else if (sendPacketState == AVERROR(EAGAIN)) {
                LOGE("向解码器发送数据失败：AVERROR(EAGAIN)");
            } else if (sendPacketState == AVERROR_EOF) {
                LOGE("向解码器发送数据失败：AVERROR_EOF");
            } else if (sendPacketState == AVERROR(EINVAL)) {
                LOGE("向解码器发送数据失败：AVERROR(EINVAL)");
            } else if (sendPacketState == AVERROR(ENOMEM)) {
                LOGE("向解码器发送数据失败：AVERROR(ENOMEM)");
            } else {
                LOGE("向解码器发送数据失败：未知");
            }
        }

        av_packet_unref(avPacket);
    }
    if(pausePlay == 0){
        currentFrame = 0;
    }
    stopPlay = 1;
    if (jcls != NULL) {
        jmethodID jmethodId = (*env)->GetMethodID(env, jcls, "onCompleted", "()V");
        (*env)->CallVoidMethod(env, instance, jmethodId, NULL);
    }
    // 内存释放
    ANativeWindow_release(nativeWindow);
    av_frame_free(&outAvFrame);
    av_frame_free(&avFrame);
    av_packet_free(&avPacket);
    avcodec_free_context(&codecContext);
    avformat_close_input(&formatContext);
    avformat_free_context(formatContext);
    (*env)->ReleaseStringUTFChars(env, videoPath, input);
}

static void * playFFmpegVideo(void *arg) {
    struct PlayParam *pstru;
    pstru = (struct PlayParam *) arg;
    LOGE("playFFmpegVideo...............");
    playVideo(pstru->env, pstru->instance, pstru->videoPath, pstru->surface);
}

void playVideoInThread(JNIEnv *env, jobject instance, jstring videoPath, jobject surface) {
    struct PlayParam *pstru = (struct PlayParam *) malloc(sizeof(struct PlayParam));
    pstru->env = env;
    pstru->instance = instance;
    pstru->videoPath = videoPath;
    pstru->surface = surface;
    LOGE("playVideoInThread111111111111111111");
    res = pthread_create(&ntid, NULL, playFFmpegVideo, (void *)pstru);
    if (res != 0) {
        LOGE("Create thread %d failed threadId:1");
        exit(res);
    }
    LOGE("playVideoInThread2222222222222222222");
    pthread_join(ntid, NULL);
    LOGE("playVideoInThread333333333333333333");
    free(pstru);
    LOGE("playVideoInThread4444444444444444");
    pthread_exit(res);
}

jlong getVideoDuration(JNIEnv *env, jobject instance, jstring videoPath) {
    const char *input = (*env)->GetStringUTFChars(env, videoPath, NULL);
    if (input == NULL) {
        LOGE("字符串转换失败......");
        return 0;
    }

    // 注册FFmpeg所有编解码器以及相关协议
    av_register_all();

    // 分配结构体
    AVFormatContext *formatContext = avformat_alloc_context();
    // 打开视频数据源 需要本地权限
    int open_state = avformat_open_input(&formatContext, input, NULL, NULL);
    if (open_state < 0) {
        char errbuf[128];
        if (av_strerror(open_state, errbuf, sizeof(errbuf) == 0)) {
            LOGE("打开视频输入流信息失败，失败原因：%s", errbuf);
        }
        return 0;
    }

    // 为分配的AVFormatContenxt结构体中填充数据
    if (avformat_find_stream_info(formatContext, NULL) < 0) {
        LOGE("读取视频输入流信息失败");
        return 0;
    }

    if (formatContext->duration != AV_NOPTS_VALUE) {
        long duration = formatContext->duration;
        LOGE("视频总时长为：%ld", duration);
        return duration / 1000;
    }
}

jstring avcodecinfo(JNIEnv *env, jobject instance) {
    char info[40000] = {0};
    av_register_all();
    AVCodec *c_temp = av_codec_next(NULL);
    while (c_temp != NULL) {
        if (c_temp->decode != NULL) {
            sprintf(info, "%sdecode:", info);
        } else {
            sprintf(info, "%sencode:", info);
        }
        switch (c_temp->type) {
            case AVMEDIA_TYPE_VIDEO:
                sprintf(info, "%s(video):", info);
                break;
            case AVMEDIA_TYPE_AUDIO:
                sprintf(info, "%s(audio):", info);
                break;
            default:
                sprintf(info, "%s(other):", info);
                break;
        }
        sprintf(info, "%s[%10s]\n", info, c_temp->name);
        c_temp = c_temp->next;
    }
    return (*env)->NewStringUTF(env, info);
}

