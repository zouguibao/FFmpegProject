package com.example.ffmpegproject.record_video.muxer;

import android.media.MediaCodec;
import android.media.MediaFormat;
import android.media.MediaMuxer;

import com.example.ffmpegproject.record_video.encode.MediaEncoder;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.text.SimpleDateFormat;
import java.util.GregorianCalendar;
import java.util.Locale;

/**
 * author:zouguibao
 * date: 2020-05-11
 * desc:
 */
public class MediaMuxerWrapper {

    private static final SimpleDateFormat mDateTimeFormat = new SimpleDateFormat("yyyy-MM-dd-HH-mm-ss", Locale.CHINA);

    private String mOutPutPath;
    private MediaMuxer mMediaMuxer;
    private int mEncoderCount;
    private int mStatredCount;
    private boolean mIsStarted;

    public MediaMuxerWrapper(String outPutPath) {
        try {
            this.mOutPutPath = outPutPath;
            this.mMediaMuxer = new MediaMuxer(outPutPath, MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4);
        } catch (IOException e) {
            e.printStackTrace();
        }
        mEncoderCount = 0;
        mStatredCount = 0;
        mIsStarted = false;
    }

    public void addEncoder(MediaEncoder mediaEncoder) {
    }

    public String getOutPutPath() {
        return mOutPutPath;
    }

    public boolean isIsStarted() {
        return mIsStarted;
    }

    public void stop() {
        mStatredCount--;
        if (mEncoderCount > 0 && mStatredCount <= 0) {
            mMediaMuxer.stop();
            mMediaMuxer.release();
        }
    }

    public synchronized int addTrack(MediaFormat mediaFormat) {
        if (!mIsStarted) {
            return mMediaMuxer.addTrack(mediaFormat);
        }
        return -1;
    }

    public void writeSampleData(int trackIndex, ByteBuffer encodedData, MediaCodec.BufferInfo bufferInfo) {
        if (mStatredCount > 0) {
            mMediaMuxer.writeSampleData(trackIndex, encodedData, bufferInfo);
        }
    }


    /**
     * get current data and time as String
     *
     * @return
     */
    public static String getDataTimeString() {
        GregorianCalendar now = new GregorianCalendar();
        return mDateTimeFormat.format(now.getTime());
    }
}
