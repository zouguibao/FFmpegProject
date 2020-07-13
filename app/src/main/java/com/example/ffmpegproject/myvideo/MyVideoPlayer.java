package com.example.ffmpegproject.myvideo;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.media.MediaCodec;
import android.media.MediaFormat;
import android.view.Surface;

import com.example.ffmpegproject.util.MyLog;

import java.nio.ByteBuffer;

/**
 * author:zouguibao
 * date: 2020-04-23
 * desc:
 */
public class MyVideoPlayer {

    private AudioTrack audioTrack;

    // 硬解类型
    private MediaFormat mediaFormat;

    // 硬解码器
    private MediaCodec mediaCodec;

    private Surface surface;

    // 硬解码器信息
    private MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();

    private boolean isSoftCodec;

    static {
        System.loadLibrary("native-lib");
    }

    public MyVideoPlayer() {

    }

    public MyVideoPlayer(Surface surface) {
        this.surface = surface;
    }

    public native void playVideo(String path, Surface surface);

    public native void playAudio(String path);

    /**
     * 创建AudioTrack
     * 由C反射调用
     *
     * @param sampleRate 采样率
     * @param channels   通道数
     */
    public void createAudioTrack(int sampleRate, int channels) {
        int channelConfig;
        if (channels == 1) {
            channelConfig = AudioFormat.CHANNEL_OUT_MONO;
        } else if (channels == 2) {
            channelConfig = AudioFormat.CHANNEL_OUT_STEREO;
        } else {
            channelConfig = AudioFormat.CHANNEL_OUT_MONO;
        }

        int bufferSize = AudioTrack.getMinBufferSize(sampleRate, channels, AudioFormat.ENCODING_PCM_16BIT);

        audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, sampleRate, channelConfig, AudioFormat.ENCODING_PCM_16BIT, bufferSize, AudioTrack.MODE_STREAM);
        audioTrack.play();
    }

    /**
     * 播放AudioTrack
     * 由C反射调用
     *
     * @param data
     * @param length
     */
    public void playAudioTrack(byte[] data, int length) {
        if (audioTrack != null && audioTrack.getPlayState() == AudioTrack.PLAYSTATE_PLAYING) {
            audioTrack.write(data, 0, length);
        }
    }

    public void releaseAudioTrack() {
        if (audioTrack != null) {
            if (audioTrack.getPlayState() == AudioTrack.PLAYSTATE_PLAYING) {
                audioTrack.stop();
            }
            audioTrack.release();
            audioTrack = null;
        }
    }

    /**
     * 播放器回调
     */
    public interface PlayerCallback {
        /**
         * 播放开始
         */
        void onStart();

        /**
         * 进度
         *
         * @param total
         * @param current
         */
        void onProgress(int total, int current);

        /**
         * 播放结束
         */
        void onEnd();
    }

    /**
     * 同步播放音视频
     *
     * @param path
     * @param surface
     * @param callback
     */
    public native void play(String path, Surface surface, PlayerCallback callback);

    /**
     * 快进/快退
     *
     * @param progress
     */
    public native void seekTo(int progress);

    /**
     * 暂停播放
     */
    public native void pause();

    /**
     * 是否暂停
     */
    public native boolean isPause();

    /**
     * 继续播放
     */
    public native void resume();

    /**
     * 停止播放
     */
    public native void stop();


    /**
     * 硬解码器初始化
     *
     * @param mimetype
     * @param width
     * @param height
     * @param csd0
     * @param csd1
     */
    public void mediacodecInit(int mimetype, int width, int height, byte[] csd0, byte[] csd1) {
        MyLog.d("mediacodecInit...............");
        try {
            String mtype = getMimeType(mimetype);
            MyLog.d("mediacodecInit............... mtype = " + mtype);
            mediaFormat = MediaFormat.createVideoFormat(mtype, width, height);
            mediaFormat.setInteger(MediaFormat.KEY_WIDTH, width);
            mediaFormat.setInteger(MediaFormat.KEY_HEIGHT, height);
            mediaFormat.setLong(MediaFormat.KEY_MAX_INPUT_SIZE, width * height);
            mediaFormat.setByteBuffer("csd-0", ByteBuffer.wrap(csd0));
            mediaFormat.setByteBuffer("csd-1", ByteBuffer.wrap(csd1));
            mediaCodec = MediaCodec.createDecoderByType(mtype);
            MyLog.d("mediacodecInit surface..............." + surface.toString());
            if (surface != null) {
                mediaCodec.stop();
                mediaCodec.configure(mediaFormat, surface, null, 0);
                mediaCodec.start();
                MyLog.d("mediacodecInit start...............");
            }
        } catch (Exception e) {
            MyLog.d("mediacodecInit Exception...............");
            e.printStackTrace();
        }
    }

    /**
     * 硬解码器解码
     *
     * @param bytes
     * @param size
     * @param pts
     */
    public void mediaCodecDecode(byte[] bytes, int size, int pts) {
        try {
            if (bytes != null && mediaCodec != null && info != null) {
                int inputBufferIndex = mediaCodec.dequeueInputBuffer(10);
//                MyLog.d("mediacodecDecode  bytes = " + bytes.toString());
//                MyLog.d("mediacodecDecode  inputBufferIndex = " + inputBufferIndex);
                if (inputBufferIndex >= 0) {
                    ByteBuffer byteBuffer = mediaCodec.getInputBuffers()[inputBufferIndex];
                    byteBuffer.clear();
                    byteBuffer.put(bytes);
                    mediaCodec.queueInputBuffer(inputBufferIndex, 0, size, pts, 0);

                    int index = mediaCodec.dequeueOutputBuffer(info, 10);
//                    MyLog.d("mediacodecDecode outputindex............... = " +index);
                    while (index >= 0) {
                        mediaCodec.releaseOutputBuffer(index, true);
                        index = mediaCodec.dequeueOutputBuffer(info, 10);
                    }
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
            MyLog.d("Exception...............");
        }
    }


    /**
     * 获取音视频类型
     *
     * @param type
     * @return
     */
    private String getMimeType(int type) {
        if (type == 1) {
            return "video/avc";
        } else if (type == 2) {
            return "video/hevc";
        } else if (type == 3) {
            return "video/mp4v-es";
        } else if (type == 4) {
            return "video/x-ms-wmv";
        }
        return "";
    }

    public boolean isSoftCodec() {
        return isSoftCodec;
    }

    public void setSoftCodec(boolean softCodec) {
        isSoftCodec = softCodec;
    }

    public void detroyMediaCodec() {
        if (mediaCodec != null) {
            mediaCodec.stop();
            mediaCodec.release();
        }
    }
}
