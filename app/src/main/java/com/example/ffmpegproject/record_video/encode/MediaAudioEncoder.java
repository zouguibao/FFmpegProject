package com.example.ffmpegproject.record_video.encode;

import com.example.ffmpegproject.record_video.listener.MediaEncoderListener;
import com.example.ffmpegproject.record_video.muxer.MediaMuxerWrapper;

/**
 * author:zouguibao
 * date: 2020-05-12
 * desc:
 */
public class MediaAudioEncoder extends MediaEncoder {
    public MediaAudioEncoder(MediaMuxerWrapper mediaMuxerWrapper, MediaEncoderListener mediaEncoderListener) {
        super(mediaMuxerWrapper, mediaEncoderListener);
    }

    @Override
    public void prepare() {

    }

}
