package com.example.ffmpegproject.record_video.listener;


import com.example.ffmpegproject.record_video.encode.MediaEncoder;

/**
 * author:zouguibao
 * date: 2020-05-12
 * desc:
 */
public interface MediaEncoderListener {
    public void onPrepared(MediaEncoder encoder);

    public void onStopped(MediaEncoder encoder);
}
