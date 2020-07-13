package com.example.ffmpegproject.record_video.encode;

import com.example.ffmpegproject.record_video.listener.MediaEncoderListener;
import com.example.ffmpegproject.record_video.muxer.MediaMuxerWrapper;

import java.io.IOException;

/**
 * author:zouguibao
 * date: 2020-05-12
 * desc:
 */
public class MediaVideoEncoder extends MediaEncoder {

    public MediaVideoEncoder(MediaMuxerWrapper mediaMuxerWrapper, MediaEncoderListener listener) {
        super(mediaMuxerWrapper, listener);
    }

    @Override
    public void prepare() {

    }


}
