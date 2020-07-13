package com.example.ffmpegproject.record.encoder;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaFormat;
import android.opengl.EGLContext;
import android.view.Surface;

import com.example.ffmpegproject.record.opengl.RenderHandler;
import com.example.ffmpegproject.util.MyLog;

import java.io.IOException;

/**
 * author:zouguibao
 * date: 2020-05-09
 * desc:
 */
public class MediaVideoEncoder extends MediaEncoder {
    private static final String TAG = "MediaVideoEncoder";
    private static final String MIME_TYPE = "video/avc";
    // parameters for recording
    private static final int FRAME_RATE = 25;
    private static final float BPP = 0.25f;
    private int mWidth;
    private int mHeight;
    private RenderHandler mRenderHandler;
    private Surface mSurface;
    /**
     * color formats that we can use in this class
     */
    protected static int[] recognizedFormats;

    static {
        recognizedFormats = new int[]{
//        	MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Planar,
//        	MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar,
//        	MediaCodecInfo.CodecCapabilities.COLOR_QCOM_FormatYUV420SemiPlanar,
                MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface,
        };
    }


    public MediaVideoEncoder(MediaMuxerWrapper mediaMuxerWrapper, MediaEncoderListener listener, int width, int height) {
        super(mediaMuxerWrapper, listener);
        this.mWidth = width;
        this.mHeight = height;
        mRenderHandler = RenderHandler.getInstance(TAG);
    }

    public boolean frameAvailableSoon(float[] tex_matrix) {
        boolean result = false;
        if (result = super.frameAvailableSoon()) {
            mRenderHandler.draw(tex_matrix);
        }
        return result;
    }

    public boolean frameAvailableSoon(final float[] tex_matrix, final float[] mvp_matrix) {
        boolean result;
        if (result = super.frameAvailableSoon())
            mRenderHandler.draw(tex_matrix, mvp_matrix);
        return result;
    }

    @Override
    public boolean frameAvailableSoon() {
        boolean result;
        if (result = super.frameAvailableSoon())
            mRenderHandler.draw(null);
        return result;
    }

    @Override
    void prepare() throws IOException {
        mTrackIndex = -1;
        mMuxerStarted = false;
        mIsEOS = false;
        MediaCodecInfo videoCodecInfo = selectVideoCodec(MIME_TYPE);

        if (videoCodecInfo == null) {
            MyLog.e("Unable to find an appropriate codec for " + MIME_TYPE);
            return;
        }
        MyLog.e("selected codec: " + videoCodecInfo.getName());

        MediaFormat format = MediaFormat.createVideoFormat(MIME_TYPE, mWidth, mHeight);
        format.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface);
        format.setInteger(MediaFormat.KEY_BIT_RATE,calcBitRate());
        format.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL,10);

        mMediaCodec = MediaCodec.createEncoderByType(MIME_TYPE);
        mMediaCodec.configure(format,null,null,MediaCodec.CONFIGURE_FLAG_ENCODE);
        // get Surface for encoder input
        // this method only can call between #configure and #start
        mSurface = mMediaCodec.createInputSurface();
        mMediaCodec.start();
        if(mListener != null){
            mListener.onPrepared(this);
        }
    }

    public void setEglContext(final EGLContext shared_context, final int tex_id) {
        mRenderHandler.setEGLContext(shared_context, tex_id, mSurface, true);
    }

    @Override
    protected void release() {
        MyLog.e("release:");
        if (mSurface != null) {
            mSurface.release();
            mSurface = null;
        }
        if (mRenderHandler != null) {
            mRenderHandler.release();
            mRenderHandler = null;
        }
        super.release();
    }

    private int calcBitRate() {
        final int bitrate = (int)(BPP * FRAME_RATE * mWidth * mHeight);
        MyLog.e(String.format("bitrate=%5.2f[Mbps]", bitrate / 1024f / 1024f));
        return bitrate;
    }

    /**
     * select the first codec that mathc a specific MIME type
     *
     * @param mimeType
     * @return
     */
    protected static MediaCodecInfo selectVideoCodec(String mimeType) {
        // get the list of available codecs
        int numCodecs = MediaCodecList.getCodecCount();
        for (int i = 0; i < numCodecs; i++) {
            MediaCodecInfo codecInfo = MediaCodecList.getCodecInfoAt(i);
            // skipped decoder
            if (!codecInfo.isEncoder()) {
                continue;
            }
            // select first codec that match a specific MIME type and color format
            String[] types = codecInfo.getSupportedTypes();
            for (int j = 0; j < types.length; j++) {
                if (types[j].equalsIgnoreCase(mimeType)) {
                    MyLog.e("codec:" + codecInfo.getName() + ",MIME=" + types[j]);
                    int format = selectColorFormat(codecInfo, mimeType);
                    if (format > 0) {
                        return codecInfo;
                    }
                }
            }
        }
        return null;
    }

    /**
     * select color format available on specific codec and we can use
     *
     * @param codecInfo
     * @param mimeType
     * @return
     */
    protected static int selectColorFormat(MediaCodecInfo codecInfo, String mimeType) {
        int result = 0;
        MediaCodecInfo.CodecCapabilities caps;
        try {
            Thread.currentThread().setPriority(Thread.MAX_PRIORITY);
            caps = codecInfo.getCapabilitiesForType(mimeType);
        } finally {
            Thread.currentThread().setPriority(Thread.NORM_PRIORITY);
        }
        int colorFormat;
        for (int i = 0; i < caps.colorFormats.length; i++) {
            colorFormat = caps.colorFormats[i];
            if (isRecognizedViewoFormat(colorFormat)) {
                if (result == 0) {
                    result = colorFormat;
                }
                break;
            }
        }
        if (result == 0) {
            MyLog.e("couldn't find a good color format for " + codecInfo.getName() + " / " + mimeType);
        }
        return result;
    }

    private static boolean isRecognizedViewoFormat(int colorFormat) {
        int n = recognizedFormats != null ? recognizedFormats.length : 0;
        for (int i = 0; i < n; i++) {
            if (recognizedFormats[i] == colorFormat) {
                return true;
            }
        }
        return false;
    }

    @Override
    protected void signalEndOfInputStream() {
        MyLog.e("sending EOS to encoder");
        mMediaCodec.signalEndOfInputStream();
        mIsEOS = true;
    }
}
