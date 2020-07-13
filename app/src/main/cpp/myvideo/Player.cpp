//
// Created by zouguibao on 2020-04-23.
//

#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <pthread.h>
#include <unistd.h>
#include "sys/types.h"


extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavutil/imgutils.h"
}

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "zouguibao", __VA_ARGS__)

/**
 * 错误打印
 * @param err
 */
void print_error(int err) {
    char err_buf[128];
    const char *err_buf_ptr = err_buf;
    if (av_strerror(err, err_buf, sizeof(err_buf_ptr)) < 0) {
        err_buf_ptr = strerror(AVUNERROR(err));
    }
    LOGE("ffmpeg error descript : %s", err_buf_ptr);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_ffmpegproject_myvideo_MyVideoPlayer_playVideo(JNIEnv *env, jobject instance,
                                                               jstring path_, jobject surface) {
    // 记录结果
    int result;
    // R1 Java String -> C String
    const char *path = env->GetStringUTFChars(path_, 0);
    // 注册 FFmpeg 组件
    av_register_all();
    avformat_network_init();
    // R2 初始化 AVFormatContext 上下文
    AVFormatContext *format_context = avformat_alloc_context();
    // 打开视频文件
    result = avformat_open_input(&format_context, path, NULL, NULL);
    if (result < 0) {
        LOGE("Player Error : Can not open video file");
        return;
    }
    // 查找视频文件的流信息
    result = avformat_find_stream_info(format_context, NULL);
    if (result < 0) {
        LOGE("Player Error : Can not find video file stream info");
        return;
    }
    // 查找视频编码器
    int video_stream_index = -1;
    for (int i = 0; i < format_context->nb_streams; i++) {
        // 匹配视频流
        if (format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }
    // 没找到视频流
    if (video_stream_index == -1) {
        LOGE("Player Error : Can not find video stream");
        return;
    }
    // 初始化视频编码器上下文
    AVCodecContext *video_codec_context = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(video_codec_context,
                                  format_context->streams[video_stream_index]->codecpar);
    // 初始化视频编码器
    AVCodec *video_codec = avcodec_find_decoder(video_codec_context->codec_id);
    if (video_codec == NULL) {
        LOGE("Player Error : Can not find video codec");
        return;
    }
    // R3 打开视频解码器
    result = avcodec_open2(video_codec_context, video_codec, NULL);
    if (result < 0) {
        LOGE("Player Error : Can not open video codec");
        return;
    }
    // 获取视频的宽高
    int videoWidth = video_codec_context->width;
    int videoHeight = video_codec_context->height;
    LOGE("playVideo......111111111 %d,%d", videoWidth, videoHeight);
    // R4 初始化 Native Window 用于播放视频
    ANativeWindow *native_window = ANativeWindow_fromSurface(env, surface);
    if (native_window == NULL) {
        LOGE("Player Error : Can not create native window");
        return;
    }
    // 通过设置宽高限制缓冲区中的像素数量，而非屏幕的物理显示尺寸。
    // 如果缓冲区与物理屏幕的显示尺寸不相符，则实际显示可能会是拉伸，或者被压缩的图像
    result = ANativeWindow_setBuffersGeometry(native_window, videoWidth, videoHeight,
                                              WINDOW_FORMAT_RGBA_8888);
    if (result < 0) {
        LOGE("Player Error : Can not set native window buffer");
        ANativeWindow_release(native_window);
        return;
    }
    // 定义绘图缓冲区
    ANativeWindow_Buffer window_buffer;
    // 声明数据容器 有3个
    // R5 解码前数据容器 Packet 编码数据
    AVPacket *packet = av_packet_alloc();
    // R6 解码后数据容器 Frame 像素数据 不能直接播放像素数据 还要转换
    AVFrame *frame = av_frame_alloc();
    // R7 转换后数据容器 这里面的数据可以用于播放
    AVFrame *rgba_frame = av_frame_alloc();
    // 数据格式转换准备
    // 输出 Buffer
    int buffer_size = av_image_get_buffer_size(AV_PIX_FMT_RGBA, videoWidth, videoHeight, 1);
    // R8 申请 Buffer 内存
    uint8_t *out_buffer = (uint8_t *) av_malloc(buffer_size * sizeof(uint8_t));
    av_image_fill_arrays(
            rgba_frame->data,
            rgba_frame->linesize,
            out_buffer,
            AV_PIX_FMT_RGBA,
            videoWidth,
            videoHeight,
            1);

    // R9 数据格式转换上下文
    struct SwsContext *data_convert_context = sws_getContext(
            videoWidth, videoHeight, video_codec_context->pix_fmt,
            videoWidth, videoHeight, AV_PIX_FMT_RGBA,
            SWS_BICUBIC, NULL, NULL, NULL);
    // 开始读取帧
    while (av_read_frame(format_context, packet) == 0) {
        // 匹配视频流
        if (packet->stream_index == video_stream_index) {
            // 解码
            result = avcodec_send_packet(video_codec_context, packet);
            if (result < 0 && result != AVERROR(EAGAIN) && result != AVERROR_EOF) {
                LOGE("Player Error : codec step 1 fail");
                return;
            }
            result = avcodec_receive_frame(video_codec_context, frame);
            if (result < 0 && result != AVERROR_EOF) {
                LOGE("Player Error : codec step 2 fail %d", result);
                return;
            }
            // 数据格式转换
            result = sws_scale(
                    data_convert_context,
                    (const uint8_t *const *) frame->data,
                    frame->linesize,
                    0,
                    videoHeight,
                    rgba_frame->data,
                    rgba_frame->linesize);
//            LOGE("Player sws_scale result = %d",result);
            if (result < 0) {
                LOGE("Player Error : data convert fail");
                return;
            }
            // 播放
            result = ANativeWindow_lock(native_window, &window_buffer, NULL);
            if (result < 0) {
                LOGE("Player Error : Can not lock native window");
            } else {
                // 将图像绘制到界面上
                // 注意 : 这里 rgba_frame 一行的像素和 window_buffer 一行的像素长度可能不一致
                // 需要转换好 否则可能花屏
                uint8_t *bits = (uint8_t *) window_buffer.bits;
                for (int h = 0; h < videoHeight; h++) {
                    memcpy(bits + h * window_buffer.stride * 4,
                           out_buffer + h * rgba_frame->linesize[0],
                           rgba_frame->linesize[0]);
                }
                ANativeWindow_unlockAndPost(native_window);
                usleep(1000 * 16);
            }
        }
        // 释放 packet 引用
        av_packet_unref(packet);
    }
    // 释放 R9
    sws_freeContext(data_convert_context);
    // 释放 R8
    av_free(out_buffer);
    // 释放 R7
    av_frame_free(&rgba_frame);
    // 释放 R6
    av_frame_free(&frame);
    // 释放 R5
    av_packet_free(&packet);
    // 释放 R4
    ANativeWindow_release(native_window);
    // 关闭 R3
    avcodec_close(video_codec_context);
    // 释放 R2
    avformat_close_input(&format_context);
    // 释放 R1
    env->ReleaseStringUTFChars(path_, path);

}


extern "C"
JNIEXPORT void JNICALL
Java_com_example_ffmpegproject_myvideo_MyVideoPlayer_playAudio(JNIEnv *env, jobject instance,
                                                               jstring path_) {
    // 记录结果
    int result;
    // R1 Java String -> C String
    const char *path = env->GetStringUTFChars(path_, 0);
    // 注册组件
    av_register_all();
    // R2 创建 AVFormatContext 上下文
    AVFormatContext *format_context = avformat_alloc_context();
    // R3 打开视频文件
    avformat_open_input(&format_context, path, NULL, NULL);
    // 查找视频文件的流信息
    result = avformat_find_stream_info(format_context, NULL);
    if (result < 0) {
        LOGE("Player Error : Can not find video file stream info");
        return;
    }
    // 查找音频编码器
    int audio_stream_index = -1;
    for (int i = 0; i < format_context->nb_streams; i++) {
        // 匹配音频流
        if (format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
        }
    }
    // 没找到音频流
    if (audio_stream_index == -1) {
        LOGE("Player Error : Can not find audio stream");
        return;
    }

    // 初始化音频编码器上下文
    AVCodecContext *audio_codec_context = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(audio_codec_context,
                                  format_context->streams[audio_stream_index]->codecpar);
    // 初始化音频编码器
    AVCodec *audio_codec = avcodec_find_decoder(audio_codec_context->codec_id);
    if (audio_codec == NULL) {
        LOGE("Player Error : Can not find audio codec");
        return;
    }
    // R4 打开视频解码器
    result = avcodec_open2(audio_codec_context, audio_codec, NULL);
    if (result < 0) {
        LOGE("Player Error : Can not open audio codec");
        return;
    }
    // 音频重采样准备
    // R5 重采样上下文
    struct SwrContext *swr_context = swr_alloc();
    // 缓冲区
    uint8_t *out_buffer = (uint8_t *) av_malloc(44100 * 2);
    // 输出的声道布局 (双通道 立体音)
    uint64_t out_channel_layout = AV_CH_LAYOUT_STEREO;
    // 输出采样位数 16位
    enum AVSampleFormat out_format = AV_SAMPLE_FMT_S16;

    // 输出的采样率必须与输入相同
    int out_sample_rate = audio_codec_context->sample_rate;
    //swr_alloc_set_opts 将PCM源文件的采样格式转换为自己希望的采样格式
    swr_alloc_set_opts(swr_context,
                       out_channel_layout,
                       out_format,
                       out_sample_rate,
                       audio_codec_context->channel_layout,
                       audio_codec_context->sample_fmt,
                       audio_codec_context->sample_rate,
                       0, NULL);
    swr_init(swr_context);
    int out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);

    // 调用Java层创建AudioTrack
    jclass player_class = env->GetObjectClass(instance);
    jmethodID create_audio_track_method_id = env->GetMethodID(player_class, "createAudioTrack",
                                                              "(II)V");
    env->CallVoidMethod(instance, create_audio_track_method_id, 44100, out_channels);

    // 播放音频准备
    jmethodID play_audio_track_method_id = env->GetMethodID(player_class, "playAudioTrack",
                                                            "([BI)V");

    // 声明数据容器 有2个
    // R6 解码前数据容器Packet 编码数据
    AVPacket *packet = av_packet_alloc();
    // R7解码后数据容器Frame MPC数据 还不能直接播放，还要进行重采样
    AVFrame *frame = av_frame_alloc();

    while (av_read_frame(format_context, packet) >= 0) {
        // 匹配音频流
        if (packet->stream_index == audio_stream_index) {
            // 解码
            result = avcodec_send_packet(audio_codec_context, packet);
            if (result < 0 && result != AVERROR(EAGAIN) && result != AVERROR_EOF) {
                LOGE("Player Error : codec step 1 fail");
                return;
            }
            result = avcodec_receive_frame(audio_codec_context, frame);
            if (result < 0 && result != AVERROR_EOF) {
                LOGE("Player Error : codec step 2 fail");
                return;
            }
            // 重采样
            swr_convert(swr_context, &out_buffer, 44100 * 2, (const uint8_t **) frame->data,
                        frame->nb_samples);
            // 播放音频 调用Java层播放AudioTrack
            int size = av_samples_get_buffer_size(NULL, out_channels, frame->nb_samples,
                                                  AV_SAMPLE_FMT_S16, 1);
            jbyteArray audio_sample_array = env->NewByteArray(size);
            env->SetByteArrayRegion(audio_sample_array, 0, size, (const jbyte *) out_buffer);
            env->CallVoidMethod(instance, play_audio_track_method_id, audio_sample_array, size);
            env->DeleteLocalRef(audio_sample_array);
        }
        // 释放 packet 引用
        av_packet_unref(packet);
    }
    // 调用 Java 层释放 AudioTrack
    jmethodID release_audio_track_method_id = env->GetMethodID(player_class, "releaseAudioTrack",
                                                               "()V");
    env->CallVoidMethod(instance, release_audio_track_method_id);
    // 释放 R7
    av_frame_free(&frame);
    // 释放 R6
    av_packet_free(&packet);
    // 释放 R5
    swr_free(&swr_context);
    // 关闭 R4
    avcodec_close(audio_codec_context);
    // 关闭 R3
    avformat_close_input(&format_context);
    // 释放 R2
    avformat_free_context(format_context);
    // 释放 R1
    env->ReleaseStringUTFChars(path_, path);
}


// 队列最大值
#define QUEUE_MAX_SIZE 50

// 节点数据类型
typedef AVPacket *NodeElement;
typedef struct _Node {
    // 数据
    NodeElement data;
    // 下一个
    struct _Node *next;
} Node;

typedef struct _Queue {
    int size;
    // 头
    Node *head;
    // 尾
    Node *tail;
    // 是否阻塞
    bool is_block;
    // 线程锁
    pthread_mutex_t *mutex_id;

    // 线程条件变量
    pthread_cond_t *not_empty_condition;
    pthread_cond_t *not_full_condition;
} Queue;

/**
 * 初始化队列
 * @param queue
 */
void queue_init(Queue *queue) {
//    LOGE("queue_init...........");
    queue->size = 0;
    queue->head = NULL;
    queue->tail = NULL;
    queue->is_block = true;
    queue->mutex_id = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(queue->mutex_id, NULL);

    queue->not_empty_condition = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
    pthread_cond_init(queue->not_empty_condition, NULL);

    queue->not_full_condition = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
    pthread_cond_init(queue->not_full_condition, NULL);
}

/**
 * 销毁队列
 * @param queue
 */
void queue_destroy(Queue *queue) {
    Node *node = queue->head;
    while (node != NULL) {
        queue->head = queue->head->next;
        free(node);
        node = queue->head;
    }

    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    queue->is_block = false;
    pthread_mutex_destroy(queue->mutex_id);
    pthread_cond_destroy(queue->not_empty_condition);
    pthread_cond_destroy(queue->not_full_condition);
    free(queue->mutex_id);
    free(queue->not_empty_condition);
    free(queue->not_full_condition);
}

/**
 * 判断是否为空
 * @param queue
 * @return
 */
bool queue_is_empty(Queue *queue) {
    return queue->size == 0;
}

/**
 * 判断是否已满
 * @param queue
 * @return
 */
bool queue_is_full(Queue *queue) {
    return queue->size == QUEUE_MAX_SIZE;
}


/**
 * 入队 (阻塞)
 * @param queue
 * @param element
 */
void queue_in(Queue *queue, NodeElement element) {
    pthread_mutex_lock(queue->mutex_id);
//    LOGE("queue_in ........... size = %d",queue->size);
    while (queue_is_full(queue) && queue->is_block) {
//        LOGE("queue_in is block...........");
        pthread_cond_wait(queue->not_full_condition, queue->mutex_id);
    }
    if (queue->size >= QUEUE_MAX_SIZE) {
        pthread_mutex_unlock(queue->mutex_id);
        return;
    }
    Node *node = (Node *) malloc(sizeof(Node));
    node->data = element;
    node->next = NULL;
    if (queue->tail == NULL) {
        queue->head = node;
        queue->tail = node;
    } else {
        queue->tail->next = node;
        queue->tail = node;
    }
    queue->size += 1;
    pthread_cond_signal(queue->not_empty_condition);
    pthread_mutex_unlock(queue->mutex_id);
}

/**
 * 出队 (阻塞)
 * @param queue
 * @return
 */
NodeElement queue_out(Queue *queue) {
    pthread_mutex_lock(queue->mutex_id);
//    LOGE("queue_out ........... size = %d",queue->size);
    while (queue_is_empty(queue) && queue->is_block) {
//        LOGE("queue_out is block...........");
        pthread_cond_wait(queue->not_empty_condition, queue->mutex_id);
    }
    if (queue->head == NULL) {
        pthread_mutex_unlock(queue->mutex_id);
        return NULL;
    }
    Node *node = queue->head;
    queue->head = queue->head->next;
    while (queue->head == NULL) {
        queue->tail = NULL;
    }
    NodeElement element = node->data;
    free(node);
    queue->size -= 1;
    pthread_cond_signal(queue->not_full_condition);
    pthread_mutex_unlock(queue->mutex_id);
    return element;
}

/**
 * 清空队列
 * @param queue
 */
void queue_clear(Queue *queue) {
    pthread_mutex_lock(queue->mutex_id);
    Node *node = queue->head;
    while (node != NULL) {
        queue->head = queue->head->next;
        free(node);
        node = queue->head;
    }
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    queue->is_block = true;
    pthread_cond_signal(queue->not_full_condition);
    pthread_mutex_unlock(queue->mutex_id);
}

/**
 * 打断阻塞
 * @param queue
 */
void break_block(Queue *queue) {
    queue->is_block = false;
    pthread_cond_signal(queue->not_empty_condition);
    pthread_cond_signal(queue->not_full_condition);
}


// 状态码
#define SUCCESS_CODE 1
#define FAIL_CODE -1

typedef struct _Player {
    // Env
    JavaVM *java_vm;
    // Java 实例
    jobject instance;
    jobject surface;
    jobject callback;

    AVFormatContext *format_context;
    // 视频相关
    int video_stream_index;
    AVCodecContext *video_codec_context;
    ANativeWindow *native_window;
    ANativeWindow_Buffer window_buffer;
    uint8_t *video_out_buffer;
    struct SwsContext *sws_context;
    AVFrame *rgba_frame;
    Queue *video_queue;

    // 音频相关
    int audio_stream_index;
    AVCodecContext *audio_codec_context;
    uint8_t *audio_out_buffer;
    struct SwrContext *swr_context;
    int out_channels;
    jmethodID play_audio_track_method_id;
    Queue *audio_queue;
    double audio_clock;
    double total;
    bool isSoftCodec = true;
} Player;

// 消费载体
typedef struct _Consumer {
    Player *player;
    int stream_index;
} Consumer;

// 播放器
Player *cplayer;

pthread_t produce_id, video_consume_id, audio_consume_id;

// 快进、快退相关
bool is_seek;
// 暂停
bool is_pause;
// 停止播放
bool is_stop;
pthread_mutex_t seek_mutex, pause_mutex, stop_mutex;
pthread_cond_t seek_condition, pause_condition, stop_condition;

/**
 * 初始化播放器
 * @param player
 * @param env
 * @param instance
 * @param surface
 * @param callback
 */
void
player_init(Player **player, JNIEnv *env, jobject instance, jobject surface, jobject callback) {
    *player = (Player *) malloc(sizeof(Player));
    JavaVM *java_vm;
    env->GetJavaVM(&java_vm);
    (*player)->java_vm = java_vm;
    (*player)->instance = env->NewGlobalRef(instance);
    (*player)->surface = env->NewGlobalRef(surface);
    (*player)->callback = env->NewGlobalRef(callback);
}

/**
 * 初始化 AVFormatContext
 * @param player
 * @param path
 * @return
 */
int format_init(Player *player, const char *path) {
//    LOGE("format_init..................");
    int result;
    av_register_all();
    player->format_context = avformat_alloc_context();

    result = avformat_open_input(&(player->format_context), path, NULL, NULL);

    if (result < 0) {
        LOGE("Player Error : Can not open video file");
        return result;
    }

    result = avformat_find_stream_info(player->format_context, NULL);
    if (result < 0) {
        LOGE("Player Error : Can not find video file stream info");
        return result;
    }
    return SUCCESS_CODE;
}

/**
 * 查找流 index
 * @param player
 * @param type
 * @return
 */
int find_stream_index(Player *player, AVMediaType type) {
//    LOGE("find_stream_index..................");
    AVFormatContext *format_context = player->format_context;
    for (int i = 0; i < format_context->nb_streams; i++) {
        if (format_context->streams[i]->codecpar->codec_type == type) {
            return i;
        }
    }
    return -1;
}

void call_is_soft_codec(Player *player) {
    JNIEnv *jniEnv;
    if (player->java_vm->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
        LOGE("call_is_soft_codec      if........");
        jclass jclz = jniEnv->GetObjectClass(player->instance);
        jmethodID jmid_is_soft_codec = jniEnv->GetMethodID(jclz, "isSoftCodec", "()Z");
        player->isSoftCodec = jniEnv->CallBooleanMethod(player->instance, jmid_is_soft_codec);
        player->java_vm->DetachCurrentThread();
    } else {
        LOGE("call_is_soft_codec      else........");
        jclass jclz = jniEnv->GetObjectClass(player->instance);
        jmethodID jmid_is_soft_codec = jniEnv->GetMethodID(jclz, "isSoftCodec", "()Z");
        player->isSoftCodec = jniEnv->CallBooleanMethod(player->instance, jmid_is_soft_codec);
    }
}

/**
 * 初始化硬件解码器
 * @param player
 * @param mimetype
 * @param width
 * @param height
 * @param csd_0_size
 * @param csd_1_size
 * @param csd_0
 * @param csd_1
 */
void call_init_mediacodec(Player *player, int mimetype, int width, int height,
                          int csd_0_size, int csd_1_size, uint8_t *csd_0, uint8_t *csd_1) {
    JNIEnv *jniEnv;
    if (player->java_vm->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
        LOGE("call_init_mediacodec      if........");
        jclass jclz = jniEnv->GetObjectClass(player->instance);
        jmethodID init_mediacodec = jniEnv->GetMethodID(jclz, "mediacodecInit", "(III[B[B)V");
        jbyteArray csd0 = jniEnv->NewByteArray(csd_0_size);
        jniEnv->SetByteArrayRegion(csd0, 0, csd_0_size, (jbyte *) csd_0);
        jbyteArray csd1 = jniEnv->NewByteArray(csd_1_size);
        jniEnv->SetByteArrayRegion(csd1, 0, csd_1_size, (jbyte *) csd_1);

        jniEnv->CallVoidMethod(player->instance, init_mediacodec, mimetype, width, height, csd0,
                               csd1);

        jniEnv->DeleteLocalRef(csd0);
        jniEnv->DeleteLocalRef(csd1);
        player->java_vm->DetachCurrentThread();
    } else {
        LOGE("call_init_mediacodec      else........");
        jclass jclz = jniEnv->GetObjectClass(player->instance);
        jmethodID init_mediacodec = jniEnv->GetMethodID(jclz, "mediacodecInit", "(III[B[B)V");
        jbyteArray csd0 = jniEnv->NewByteArray(csd_0_size);
        jniEnv->SetByteArrayRegion(csd0, 0, csd_0_size, (jbyte *) csd_0);
        jbyteArray csd1 = jniEnv->NewByteArray(csd_1_size);
        jniEnv->SetByteArrayRegion(csd1, 0, csd_1_size, (jbyte *) csd_1);

        jniEnv->CallVoidMethod(player->instance, init_mediacodec, mimetype, width, height, csd0,
                               csd1);

        jniEnv->DeleteLocalRef(csd0);
        jniEnv->DeleteLocalRef(csd1);
    }
}

int getMimeType(const char *codecName) {

    if (strcmp(codecName, "h264") == 0) {
        return 1;
    }
    if (strcmp(codecName, "hevc") == 0) {
        return 2;
    }
    if (strcmp(codecName, "mpeg4") == 0) {
        return 3;
    }
    if (strcmp(codecName, "wmv3") == 0) {
        return 4;
    }

    return -1;
}


/**
 * 初始化解码器
 * @param player
 * @param type
 * @return
 */
int codec_init(Player *player, AVMediaType type) {
//    LOGE("codec_init..................");
    int result;
    AVFormatContext *format_context = player->format_context;
    int index = find_stream_index(player, type);
    if (index == -1) {
        LOGE("Player Error : Can not find stream");
        return FAIL_CODE;
    }
    AVCodecContext *codec_context = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(codec_context, format_context->streams[index]->codecpar);
    AVCodec *codec = avcodec_find_decoder(codec_context->codec_id);
    result = avcodec_open2(codec_context, codec, NULL);
    if (result < 0) {
        LOGE("Player Error : Can not open codec");
        return FAIL_CODE;
    }
    if (type == AVMEDIA_TYPE_VIDEO) {
        player->video_stream_index = index;
        player->video_codec_context = codec_context;
        call_is_soft_codec(player);
        if (!player->isSoftCodec) {
            call_init_mediacodec(player,
                                 getMimeType(codec_context->codec->name),
                                 player->video_codec_context->width,
                                 player->video_codec_context->height,
                                 player->video_codec_context->extradata_size,
                                 player->video_codec_context->extradata_size,
                                 player->video_codec_context->extradata,
                                 player->video_codec_context->extradata
            );
        }
    } else if (type == AVMEDIA_TYPE_AUDIO) {
        player->audio_stream_index = index;
        player->audio_codec_context = codec_context;
    }


    LOGE("isSoftCodec = %d", player->isSoftCodec);
    return SUCCESS_CODE;
}

/**
 * 播放器准备中
 * @param player
 * @param env
 * @return
 */
int video_prepare(Player *player, JNIEnv *env) {
    AVCodecContext *codec_context = player->video_codec_context;
    int videoWidth = codec_context->width;
    int videoHeight = codec_context->height;
//    LOGE("video_prepare......111111111 %d,%d", videoWidth, videoHeight);
    player->native_window = ANativeWindow_fromSurface(env, player->surface);

    if (player->native_window == NULL) {
        LOGE("Player Error : Can not create native window");
        return FAIL_CODE;
    }
//    LOGE("video_prepare......2222222222222222");
    int result = ANativeWindow_setBuffersGeometry(player->native_window, videoWidth, videoHeight,
                                                  WINDOW_FORMAT_RGBA_8888);

    if (result < 0) {
        LOGE("Player Error : Can not set native window buffer");
        ANativeWindow_release(player->native_window);
        return FAIL_CODE;
    }
//    LOGE("video_prepare......33333333333333333");
    player->rgba_frame = av_frame_alloc();
    int buffer_size = av_image_get_buffer_size(AV_PIX_FMT_RGBA, videoWidth, videoHeight, 1);
    player->video_out_buffer = (uint8_t *) av_malloc(buffer_size * sizeof(uint8_t));
//    LOGE("video_prepare......444444444444444");
    av_image_fill_arrays(player->rgba_frame->data,
                         player->rgba_frame->linesize,
                         player->video_out_buffer,
                         AV_PIX_FMT_RGBA,
                         videoWidth, videoHeight, 1);
//    LOGE("video_prepare......5555555555555555");
    player->sws_context = sws_getContext(
            codec_context->width, codec_context->height, codec_context->pix_fmt,
            codec_context->width, codec_context->height, AV_PIX_FMT_RGBA,
            SWS_BICUBIC, NULL, NULL, NULL);
//    LOGE("video_prepare......666666666666666666");
    return SUCCESS_CODE;

}

int audio_prepare(Player *player, JNIEnv *env) {
    AVCodecContext *codec_context = player->audio_codec_context;
    player->swr_context = swr_alloc();
    player->audio_out_buffer = (uint8_t *) av_malloc(44100 * 2);
    uint64_t out_channel_layout = AV_CH_LAYOUT_STEREO;
    enum AVSampleFormat out_format = AV_SAMPLE_FMT_S16;
    int out_sample_rate = player->audio_codec_context->sample_rate;
    swr_alloc_set_opts(player->swr_context,
                       out_channel_layout, out_format, out_sample_rate,
                       codec_context->channel_layout, codec_context->sample_fmt,
                       codec_context->sample_rate,
                       0, NULL);
    swr_init(player->swr_context);
    player->out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    jclass player_class = env->GetObjectClass(player->instance);
    jmethodID create_audio_track_method_id = env->GetMethodID(player_class, "createAudioTrack",
                                                              "(II)V");
    env->CallVoidMethod(player->instance, create_audio_track_method_id, 44100,
                        player->out_channels);
    player->play_audio_track_method_id = env->GetMethodID(player_class, "playAudioTrack", "([BI)V");
    return SUCCESS_CODE;
}


/**
 * 视频播放
 * @param frame
 */
void video_play(Player *player, AVFrame *frame, JNIEnv *env) {
//    LOGE("video_play................");
    int video_height = player->video_codec_context->height;
    int result = sws_scale(
            player->sws_context,
            (const uint8_t *const *) frame->data, frame->linesize,
            0, video_height,
            player->rgba_frame->data, player->rgba_frame->linesize);
    if (result < 0) {
        LOGE("Player Error : video data convert fail");
        return;
    }
    result = ANativeWindow_lock(player->native_window, &(player->window_buffer), NULL);
    if (result < 0) {
        LOGE("Player Error : Can not lock native window");
    } else {
        uint8_t *bits = (uint8_t *) player->window_buffer.bits;
//        LOGE("video_play start video_height = %d",video_height);
        for (int h = 0; h < video_height; h++) {
            memcpy(bits + h * player->window_buffer.stride * 4,
                   player->video_out_buffer + h * player->rgba_frame->linesize[0],
                   player->rgba_frame->linesize[0]);
        }
//        LOGE("video_play end video_height = %d",video_height);
        ANativeWindow_unlockAndPost(player->native_window);
    }
}


/**
 * 音频播放
 * @param frame
 */
void audio_play(Player *player, AVFrame *frame, JNIEnv *env) {
//    LOGE("audio_play................");
    swr_convert(player->swr_context, &(player->audio_out_buffer), 44100 * 2,
                (const uint8_t **) frame->data, frame->nb_samples);
    int size = av_samples_get_buffer_size(NULL, player->out_channels, frame->nb_samples,
                                          AV_SAMPLE_FMT_S16, 1);
    jbyteArray audio_sample_array = env->NewByteArray(size);
    env->SetByteArrayRegion(audio_sample_array, 0, size, (const jbyte *) player->audio_out_buffer);
    env->CallVoidMethod(player->instance, player->play_audio_track_method_id, audio_sample_array,
                        size);
    env->DeleteLocalRef(audio_sample_array);
}

/**
 * 释放播放器
 * @param player
 */
void player_release(Player *player) {
    LOGE("player_release...........1111111111111");
    avformat_close_input(&(player->format_context));
    av_free(player->video_out_buffer);
    av_free(player->audio_out_buffer);
    avcodec_close(player->video_codec_context);
    ANativeWindow_release(player->native_window);
    sws_freeContext(player->sws_context);
    av_frame_free(&(player->rgba_frame));
    avcodec_close(player->audio_codec_context);
    swr_free(&(player->swr_context));
    queue_destroy(player->video_queue);
    queue_destroy(player->audio_queue);
    player->instance = NULL;
    JNIEnv *env;
    int result = player->java_vm->AttachCurrentThread(&env, NULL);
    if (result != JNI_OK) {
        LOGE("player_release...........222222222222");
        return;
    }
    env->DeleteGlobalRef(player->instance);
    env->DeleteGlobalRef(player->surface);
    player->java_vm->DetachCurrentThread();
}


/**
 * 回调 Java Callback onStart方法
 * @param player
 */
void call_on_start(Player *player, JNIEnv *env) {
    jclass callback_class = env->GetObjectClass(player->callback);
    jmethodID on_start_method_id = env->GetMethodID(callback_class, "onStart", "()V");
    env->CallVoidMethod(player->callback, on_start_method_id);
    env->DeleteLocalRef(callback_class);
//    LOGE("call_on_start...........");

}

/**
 * 回调 Java Callback onStart方法
 * @param player
 */
void call_on_end(Player *player, JNIEnv *env) {
    jclass callback_class = env->GetObjectClass(player->callback);
    jmethodID on_end_method_id = env->GetMethodID(callback_class, "onEnd", "()V");
    env->CallVoidMethod(player->callback, on_end_method_id);
    env->DeleteLocalRef(callback_class);
//    LOGE("call_on_end...........");
}


/**
 * 回调 Java Callback onStart方法
 * @param player
 * @param env
 * @param total
 * @param current
 */
void call_on_progress(Player *player, JNIEnv *env, double total, double current) {
    jclass callback_class = env->GetObjectClass(player->callback);
    jmethodID on_progress_method_id = env->GetMethodID(callback_class, "onProgress", "(II)V");
    env->CallVoidMethod(player->callback, on_progress_method_id, (int) total, (int) current);
    env->DeleteLocalRef(callback_class);
//    LOGE("call_on_progress...........");
}


/**
 * 硬解码
 * @param player
 * @param size
 * @param packet_data
 * @param pts
 */
void call_decode_mediacodec(Player *player, int size, uint8_t *packet_data, int pts) {
    JNIEnv *jniEnv;
    if (player->java_vm->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
//        LOGE("call_decode_mediacodec  if...........");
        jclass jclz = jniEnv->GetObjectClass(player->instance);
        jmethodID decode_mediacodec = jniEnv->GetMethodID(jclz, "mediaCodecDecode", "([BII)V");
        jbyteArray data = jniEnv->NewByteArray(size);
        jniEnv->SetByteArrayRegion(data, 0, size, (jbyte *) packet_data);
        jniEnv->CallVoidMethod(player->instance, decode_mediacodec, data, size, pts);
        jniEnv->DeleteLocalRef(data);
        player->java_vm->DetachCurrentThread();

    } else {
//        LOGE("call_decode_mediacodec  else...........");
        jclass jclz = jniEnv->GetObjectClass(player->instance);
        jmethodID decode_mediacodec = jniEnv->GetMethodID(jclz, "mediaCodecDecode", "([BII)V");
        jbyteArray data = jniEnv->NewByteArray(size);
        jniEnv->SetByteArrayRegion(data, 0, size, (jbyte *) packet_data);
        jniEnv->CallVoidMethod(player->instance, decode_mediacodec, data, size, pts);
        jniEnv->DeleteLocalRef(data);
    }
}

/**
 * 生产函数,用于线程中
 * 循环读取帧 解码 丢到对应的队列中
 * @param arg
 * @return
 */
void *produce(void *arg) {
    Player *player = (Player *) arg;
    AVPacket *packet = av_packet_alloc();
    int mimeType = getMimeType(player->video_codec_context->codec->name);
//    LOGE("mimeType = %d" , mimeType);
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
//    LOGE("produce stream index = %d",player->video_stream_index);
//    LOGE("produce stream index = %d",player->audio_stream_index);
//    LOGE("produce...........");

    for (;;) {
//        LOGE("produce...........生产开始了");
//        pthread_mutex_lock(&stop_mutex);
//        while (is_stop) {
//            pthread_cond_wait(&stop_condition, &stop_mutex);
//        }
        if (is_stop) {
            break;
        }
//        pthread_mutex_unlock(&stop_mutex);

        pthread_mutex_lock(&pause_mutex);
        while (is_pause) {
            pthread_cond_wait(&pause_condition, &pause_mutex);
        }
        pthread_mutex_unlock(&pause_mutex);

        pthread_mutex_lock(&seek_mutex);
//        LOGE("produce...........333333333333333");
        while (is_seek) {
            LOGE("Player Log : produce waiting seek");
            pthread_cond_wait(&seek_condition, &seek_mutex);
            LOGE("Player Log : produce wake up seek");
        }
        pthread_mutex_unlock(&seek_mutex);
        int result = av_read_frame(player->format_context, packet);
        // 读取数据流的下一帧
        if (result < 0) {
            LOGE("Player Log : read next stream fail!");
//            LOGE("produce...........22222222222222222");
            break;
        }
//        LOGE("produce...........4444444444444444");
        if (packet->stream_index == player->video_stream_index) {
            if (!player->isSoftCodec || mimeType != -1) {
                // 硬解码
                AVPacket newPacket;
                av_init_packet(&newPacket);
                // 将原始的avcc格式转换为Annexb格式，，使其符合MediaCodec的要求
                int ret = av_bitstream_filter_filter(mimType,
                                                     player->video_codec_context,
                                                     NULL, &newPacket.data, &newPacket.size,
                                                     packet->data,
                                                     packet->size, 0);
                if (ret >= 0) {
                    packet->data = newPacket.data;
                } else {
                    LOGE("covert bitstram fail!");
                }
            }
            queue_in(player->video_queue, packet);
//            LOGE("produce...........video_stream_index");
        } else if (packet->stream_index == player->audio_stream_index) {
            queue_in(player->audio_queue, packet);
//            LOGE("produce...........audio_stream_index");
        }
        packet = av_packet_alloc();
    }
    LOGE("produce...........生产结束了");
    break_block(player->video_queue);
    break_block(player->audio_queue);
    av_bitstream_filter_close(mimType);
    LOGE("produce...........55555555555555555");
    for (;;) {
        LOGE("produce...........videoqueue size = %d,audioqueue size = %d",
             player->video_queue->size, player->audio_queue->size);
        if (queue_is_empty(player->video_queue) && queue_is_empty(player->audio_queue)) {
            LOGE("produce...........video_queue and audio_queue both null");
            break;
        }
        sleep(1);
    }
    player_release(player);
    return NULL;
}

/* no AV sync correction is done if below the minimum AV sync threshold */
#define AV_SYNC_THRESHOLD_MIN 0.04
/* AV sync correction is done if above the maximum AV sync threshold */
#define AV_SYNC_THRESHOLD_MAX 0.1
/* If a frame duration is longer than this, it will not be duplicated to compensate AV sync */
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0


/**
 * 消费函数
 * 从队列获取解码数据 同步播放
 * @param arg
 * @return
 */
void *comnsume(void *arg) {
//    LOGE("comnsume...........");
    Consumer *consumer = (Consumer *) arg;
    Player *player = consumer->player;
    int index = consumer->stream_index;
//    LOGE("comnsume........... video = %d",player->video_stream_index);
//    LOGE("comnsume........... audio = %d",player->audio_stream_index);
    JNIEnv *env;
    int result = player->java_vm->AttachCurrentThread(&env, NULL);
//    LOGE("comnsume...........1111111111");
    if (result != JNI_OK) {
        LOGE("Player Error : Can not get current thread env");
        pthread_exit(NULL);
//        return NULL;
    }
    AVCodecContext *codec_context;
    AVStream *stream;
    Queue *queue;
    if (index == player->video_stream_index) {
//        LOGE("comnsume...........video_stream_index  start");
        codec_context = player->video_codec_context;
        stream = player->format_context->streams[player->video_stream_index];
        queue = player->video_queue;
//        LOGE("comnsume...........video_stream_index  start %d", player->video_stream_index);
        video_prepare(player, env);
//        LOGE("comnsume...........video_stream_index  end");
    } else if (index == player->audio_stream_index) {
//        LOGE("comnsume...........audio_stream_index");
        codec_context = player->audio_codec_context;
        stream = player->format_context->streams[player->audio_stream_index];
        queue = player->audio_queue;
        audio_prepare(player, env);
        call_on_start(player, env);
    }
//    LOGE("comnsume...........55555555555555");
    double total = stream->duration * av_q2d(stream->time_base);
    player->total = total;
//    LOGE("comnsume...........total = %f", total);
    AVFrame *frame = av_frame_alloc();
    for (;;) {
//        LOGE("comnsume...........循环播放开始了");
        pthread_mutex_lock(&pause_mutex);
        while (is_pause) {
            pthread_cond_wait(&pause_condition, &pause_mutex);
        }
        pthread_mutex_unlock(&pause_mutex);
        pthread_mutex_lock(&seek_mutex);
        while (is_seek) {
            LOGE("Player Log : consumer waiting seek");
            pthread_cond_wait(&seek_condition, &seek_mutex);
            LOGE("Player Log : consumer wake up seek");
        }
        pthread_mutex_unlock(&seek_mutex);

        AVPacket *packet = queue_out(queue);

        if (packet == NULL) {
            LOGE("consume packet is null");
            break;
        }
        result = avcodec_send_packet(codec_context, packet);
//        LOGE("comnsume...........22222222222222222 result = %d", result);
        if (result < 0 && result != AVERROR(EAGAIN) && result != AVERROR_EOF) {
            print_error(result);
            LOGE("Player Error : %d codec step 1 fail", index);
            av_packet_free(&packet);
            continue;
        }

        result = avcodec_receive_frame(codec_context, frame);
//        LOGE("comnsume...........33333333333333333 result = %d", result);
        if (result < 0 && result != AVERROR_EOF) {
            print_error(result);
            LOGE("Player Error : %d codec step 2 fail", index);
            av_packet_free(&packet);
            continue;
        }

        if (index == player->video_stream_index) {
            // getCurrentTime
            double audio_clock = player->audio_clock;
            double timestamp;
            if (packet->pts == AV_NOPTS_VALUE) {
                timestamp = 0;
            } else {
                timestamp = av_frame_get_best_effort_timestamp(frame) * av_q2d(stream->time_base);
            }
            double frame_rate = av_q2d(stream->avg_frame_rate);
            frame_rate += frame->repeat_pict * (frame_rate * 0.5);
            if (timestamp == 0.0) {
                usleep((unsigned long) (frame_rate * 1000));
            } else {
                if (fabs(timestamp - audio_clock) > AV_SYNC_THRESHOLD_MIN &&
                    fabs(timestamp - audio_clock) < AV_NOSYNC_THRESHOLD) {
                    if (timestamp > audio_clock) {
                        usleep((unsigned long) ((timestamp - audio_clock) * 1000000));
                    }
                }
            }
            if (player->isSoftCodec) {
                // 软解码
                video_play(player, frame, env);
            } else {
                call_decode_mediacodec(player, packet->size, packet->data, 0);
            }

        } else if (index == player->audio_stream_index) {
            player->audio_clock = packet->pts * av_q2d(stream->time_base);
            audio_play(player, frame, env);
            call_on_progress(player, env, total, player->audio_clock);
        }
        av_packet_free(&packet);
    }
    LOGE("comnsume...........循环播放结束了");
    if (index == player->audio_stream_index) {
        call_on_end(player, env);
    }
    player->java_vm->DetachCurrentThread();
    return NULL;
}

/**
 *  初始化线程
 */
void thread_init(Player *player) {
//    LOGE("thread_init...........");
    pthread_create(&produce_id, NULL, produce, player);

    Consumer *video_consumer = (Consumer *) malloc(sizeof(Consumer));
    video_consumer->player = player;
    video_consumer->stream_index = player->video_stream_index;
    pthread_create(&video_consume_id, NULL, comnsume, video_consumer);

    Consumer *audio_consumer = (Consumer *) malloc(sizeof(Consumer));
    audio_consumer->player = player;
    audio_consumer->stream_index = player->audio_stream_index;
    pthread_create(&audio_consume_id, NULL, comnsume, audio_consumer);

    pthread_mutex_init(&seek_mutex, NULL);
    pthread_cond_init(&seek_condition, NULL);

    pthread_mutex_init(&pause_mutex, NULL);
    pthread_cond_init(&pause_condition, NULL);

    pthread_mutex_init(&stop_mutex, NULL);
    pthread_cond_init(&stop_condition, NULL);
}

/**
 * 开始播放
 * @param player
 */
void play_start(Player *player) {
    is_stop = false;
    player->audio_clock = 0;
    player->video_queue = (Queue *) malloc(sizeof(Queue));
    player->audio_queue = (Queue *) malloc(sizeof(Queue));
    queue_init(player->video_queue);
    queue_init(player->audio_queue);
    thread_init(player);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_ffmpegproject_myvideo_MyVideoPlayer_play(JNIEnv *env, jobject instance,
                                                          jstring path_, jobject surface,
                                                          jobject callback) {
    const char *path = env->GetStringUTFChars(path_, 0);
    LOGE("play path = %s", path);
    int result = 1;
    Player *player;
    player_init(&player, env, instance, surface, callback);
//    LOGE("1111111111111111111111");
    if (result > 0) {
//        LOGE("2222222222222222222222");
        result = format_init(player, path);
    }
    if (result > 0) {
//        LOGE("333333333333333333333 AVMEDIA_TYPE_VIDEO");
        result = codec_init(player, AVMEDIA_TYPE_VIDEO);
    }
    if (result > 0) {
//        LOGE("444444444444444444444  AVMEDIA_TYPE_AUDIO");
        result = codec_init(player, AVMEDIA_TYPE_AUDIO);
    }
    if (result > 0) {
//        LOGE("55555555555555555555");
        play_start(player);
    }
    env->ReleaseStringUTFChars(path_, path);
    cplayer = player;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_ffmpegproject_myvideo_MyVideoPlayer_seekTo(JNIEnv *env, jobject instance,
                                                            jint progress) {
    is_seek = true;
    pthread_mutex_lock(&seek_mutex);
    queue_clear(cplayer->video_queue);
    queue_clear(cplayer->audio_queue);
//    int result = av_seek_frame(cplayer->format_context, cplayer->video_stream_index,
//                               (int64_t) (progress /
//                                          av_q2d(cplayer->format_context->streams[cplayer->video_stream_index]->time_base)),
//                               AVSEEK_FLAG_BACKWARD);
//    if (result < 0) {
//        LOGE("Player Error : Can not seek video to %d", progress);
//        return;
//    }
//    result = av_seek_frame(cplayer->format_context, cplayer->audio_stream_index,
//                           (int64_t) (progress /
//                                      av_q2d(cplayer->format_context->streams[cplayer->audio_stream_index]->time_base)),
//                           AVSEEK_FLAG_BACKWARD);
//    if (result < 0) {
//        LOGE("Player Error : Can not seek audio to %d", progress);
//        return;
//    }
    int64_t rel = progress * AV_TIME_BASE;
    int ret = avformat_seek_file(cplayer->format_context, -1, INT64_MIN, rel, INT64_MAX, 0);
    LOGE("seek to ret = %d", ret);
//    if (ret >= 0) {
//        avcodec_flush_buffers(cplayer->video_codec_context);
//        avcodec_flush_buffers(cplayer->audio_codec_context);
//    }
    is_seek = false;
    pthread_cond_broadcast(&seek_condition);
    pthread_mutex_unlock(&seek_mutex);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_ffmpegproject_myvideo_MyVideoPlayer_pause(JNIEnv *env, jobject instance) {
    if (cplayer != NULL) {
        is_pause = true;
    }
}

extern "C"
JNIEXPORT bool JNICALL
Java_com_example_ffmpegproject_myvideo_MyVideoPlayer_isPause(JNIEnv *env, jobject instance) {
    return cplayer != NULL && is_pause;
}


extern "C"
JNIEXPORT void JNICALL
Java_com_example_ffmpegproject_myvideo_MyVideoPlayer_resume(JNIEnv *env, jobject instance) {
    if (cplayer != NULL) {
        pthread_mutex_lock(&pause_mutex);
        is_pause = false;
        pthread_cond_broadcast(&pause_condition);
        pthread_mutex_unlock(&pause_mutex);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_ffmpegproject_myvideo_MyVideoPlayer_stop(JNIEnv *env, jobject instance) {
    pthread_mutex_lock(&stop_mutex);
    is_stop = true;
    pthread_cond_broadcast(&stop_condition);
    pthread_mutex_unlock(&stop_mutex);
}