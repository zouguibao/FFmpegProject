package com.example.ffmpegproject.record.encoder;

import android.media.MediaCodec;
import android.media.MediaFormat;
import android.media.MediaMuxer;
import android.os.Environment;

import com.example.ffmpegproject.util.MyLog;

import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.text.SimpleDateFormat;
import java.util.GregorianCalendar;
import java.util.Locale;

/**
 * author:zouguibao
 * date: 2020-05-09
 * desc:
 */
public class MediaMuxerWrapper {

    private static final String DIR_NAME = "AVRecSample";
    private static final SimpleDateFormat mDateTimeFormat = new SimpleDateFormat("yyyy-MM-dd-HH-mm-ss", Locale.CHINA);

    private String mOutputPath;
    private MediaMuxer mMediaMuxer;    // API >= 18
    private int mEncoderCount, mStatredCount;
    private boolean mIsStarted;
    private MediaEncoder mVideoEncoder, mAudioEncoder;

    /**
     * ext extension of output file
     *
     * @param ext
     */
    public MediaMuxerWrapper(String ext) {
        try {
            mOutputPath = getCaptureFile(Environment.DIRECTORY_MOVIES, ext).toString();
            mMediaMuxer = new MediaMuxer(mOutputPath, MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4);
        } catch (IOException e) {
            e.printStackTrace();
        }
        mEncoderCount = 0;
        mStatredCount = 0;
        mIsStarted = false;
    }

    public void prepare() throws IOException {
        if (mVideoEncoder != null) {
            mVideoEncoder.prepare();
        }
        if (mAudioEncoder != null) {
            mAudioEncoder.prepare();
        }
    }

    public void startRecording() {
        if (mVideoEncoder != null) {
            mVideoEncoder.startRecording();
        }
        if (mAudioEncoder != null) {
            mAudioEncoder.startRecording();
        }
    }

    public void stopRecording() {
        if (mVideoEncoder != null) {
            mVideoEncoder.stopRecording();
        }
        if (mAudioEncoder != null) {
            mAudioEncoder.stopRecording();
        }

        mVideoEncoder = null;
        mAudioEncoder = null;
    }

    public synchronized boolean isStarted() {
        return mIsStarted;
    }

    /**
     * assign encoder to this class. this is called from encoder
     *
     * @param mediaEncoder encoder instance of MediaVideoEncoder or MediaAudioEncoder
     */
    public void addEncoder(MediaEncoder mediaEncoder) {
        if (mediaEncoder instanceof MediaVideoEncoder && mVideoEncoder == null) {
            if (mVideoEncoder != null)
                throw new IllegalArgumentException("Video encoder already added.");
            mVideoEncoder = mediaEncoder;
        } else if (mediaEncoder instanceof MediaAudioEncoder && mAudioEncoder == null) {
            if (mAudioEncoder != null)
                throw new IllegalArgumentException("Video encoder already added.");
            mAudioEncoder = mediaEncoder;
        } else {
            throw new IllegalArgumentException("unsupported encoder");
        }
        mEncoderCount = (mVideoEncoder != null ? 1 : 0) + (mAudioEncoder != null ? 1 : 0);
    }

    public String getOutputPath() {
        return mOutputPath;
    }

    /**
     * request stop recording from encoder when encoder received EOS
     */
    public void stop() {
        mStatredCount--;
        if (mEncoderCount > 0 && mStatredCount <= 0) {
            mMediaMuxer.stop();
            mMediaMuxer.release();
            mIsStarted = false;
        }
    }

    /**
     * assign encoder to muxer
     *
     * @param format
     * @return minus value indicate error
     */
    public synchronized int addTrack(MediaFormat format) {
        if (mIsStarted)
            throw new IllegalStateException("muxer already started");
        int trackIx = mMediaMuxer.addTrack(format);
        MyLog.e("addTrack:trackNum=" + mEncoderCount + ",trackIx=" + trackIx + ",format=" + format);
        return trackIx;
    }

    /**
     * request start recording from encoder
     *
     * @return true when muxer is ready to write
     */
    public boolean start() {
        mStatredCount++;
        if (mEncoderCount > 0 && mStatredCount == mEncoderCount) {
            mMediaMuxer.start();
            mIsStarted = true;
            notifyAll();
        }
        return mIsStarted;
    }

    /**
     * write encoded data to muxer
     *
     * @param trackIndex
     * @param encodedData
     * @param bufferInfo
     */
    public void writeSampleData(int trackIndex, ByteBuffer encodedData, MediaCodec.BufferInfo bufferInfo) {
        if (mStatredCount > 0) {
            mMediaMuxer.writeSampleData(trackIndex, encodedData, bufferInfo);
        }
    }

    public static File getCaptureFile(String type, String ext) {
        File dirFile = new File(Environment.getExternalStoragePublicDirectory(type), DIR_NAME);
        dirFile.mkdirs();
        if (dirFile.canWrite()) {
            return new File(dirFile, getDataTimeString() + ext);
        }
        return null;
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
