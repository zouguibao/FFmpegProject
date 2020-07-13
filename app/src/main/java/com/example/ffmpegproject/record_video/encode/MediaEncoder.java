package com.example.ffmpegproject.record_video.encode;

import android.media.MediaCodec;

import com.example.ffmpegproject.record_video.listener.MediaEncoderListener;
import com.example.ffmpegproject.record_video.muxer.MediaMuxerWrapper;
import com.example.ffmpegproject.util.MyLog;

/**
 * author:zouguibao
 * date: 2020-05-11
 * desc:
 */
public abstract class MediaEncoder implements Runnable {

    protected static final int TIMEOUT_USEC = 10000;    // 10[msec]
    protected static final int MSG_FRAME_AVAILABLE = 1;
    protected static final int MSG_STOP_RECORDING = 9;

    protected final Object mSync = new Object();

    private MediaMuxerWrapper mMediaMuxerWrapper;
    private MediaEncoderListener mMediaEncoderListener;

    /**
     * Flag that indicate encoder received EOS(End Of Stream)
     */
    protected boolean mIsEOS;

    /**
     * Flag the indicate the muxer is running
     */
    protected boolean mMuxerStarted;

    /**
     * Flag to request stop capturing
     */
    protected volatile boolean mRequestStop;

    /**
     * Flag that indicate this encoder is capturing now.
     */
    protected volatile boolean mIsCapturing;

    /**
     * Flag that indicate the frame data will be available soon.
     */
    private int mRequestDrain;

    /**
     * Track Number
     */
    protected int mTrackIndex;

    /**
     * MediaCodec instance for encoding
     */
    protected MediaCodec mMediaCodec;

    /**
     * BufferInfo instance for dequeuing
     */
    private MediaCodec.BufferInfo mBufferInfo;        // API >= 16(Android4.1.2)

    /**
     * previous presentationTimeUs for writing
     */
    private long prevOutputPTSUs = 0;


    public MediaEncoder(MediaMuxerWrapper mediaMuxerWrapper, MediaEncoderListener mediaEncoderListener) {
        this.mMediaMuxerWrapper = mediaMuxerWrapper;
        this.mMediaEncoderListener = mediaEncoderListener;
        this.mMediaMuxerWrapper.addEncoder(this);

        synchronized (mSync) {
            mBufferInfo = new MediaCodec.BufferInfo();
            new Thread(this, getClass().getSimpleName()).start();
            try {
                mSync.wait();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    @Override
    public void run() {

        synchronized (mSync) {
            mRequestStop = false;
            mRequestDrain = 0;
            mSync.notify();
        }
        final boolean isRunning = true;
        boolean localRequestStop;
        boolean localRequestDrain;
        while (isRunning) {
            synchronized (mSync) {
                localRequestStop = mRequestStop;
                localRequestDrain = (mRequestDrain > 0);
                if (localRequestDrain)
                    mRequestDrain--;
            }
            if (localRequestStop) {

            }

            if (localRequestDrain) {

            }
        }
        MyLog.e("Encoder thread exiting");
        synchronized (mSync) {
            mRequestStop = true;
            mIsCapturing = false;
        }
    }

    public abstract void prepare();


    /**
     * get next encoding presentationTimeUs
     *
     * @return
     */
    protected long getPTSUs() {
        long result = System.nanoTime() / 1000L;
        // presentationTimeUs should be monotonic
        // otherwise muxer fail to write
        if (result < prevOutputPTSUs)
            result = (prevOutputPTSUs - result) + result;
        return result;
    }


    /**
     * the method to indicate frame data is soon available or already available
     *
     * @return return true if encoder is ready to encod.
     */
    public boolean frameAvailableSoon() {
        MyLog.e("frameAvailableSoon");
        synchronized (mSync) {
            if (!mIsCapturing || mRequestStop) {
                return false;
            }
            mRequestDrain++;
            mSync.notifyAll();
        }
        return true;
    }

    /**
     * 唤醒编码线程
     */
    public void startRecording() {
        synchronized (mSync) {
            mIsCapturing = true;
            mRequestStop = false;
            mSync.notifyAll();
        }
    }

    public void stopRecording() {
        MyLog.e("stopRecording");
        synchronized (mSync) {
            if (!mIsCapturing || mRequestStop) {
                return;
            }
            mRequestStop = true;    // for rejecting newer frame
            mSync.notifyAll();
            // We can not know when the encoding and writing finish.
            // so we return immediately after request to avoid delay of caller thread
        }
    }

    public void release() {
        mMediaEncoderListener.onStopped(this);
        if (mMediaMuxerWrapper != null && mMuxerStarted) {
            mMediaMuxerWrapper.stop();
        }
        if (mMediaCodec != null) {
            mMediaCodec.stop();
            mMediaCodec.release();
        }
        mBufferInfo = null;
    }

}
