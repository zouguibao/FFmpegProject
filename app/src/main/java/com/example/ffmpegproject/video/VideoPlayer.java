package com.example.ffmpegproject.video;

import android.graphics.Bitmap;
import android.media.MediaCodec;
import android.media.MediaFormat;
import android.text.TextUtils;
import android.util.Log;
import android.view.Surface;

import com.example.ffmpegproject.bean.WlTimeBean;
import com.example.ffmpegproject.listener.WlOnCompleteListener;
import com.example.ffmpegproject.listener.WlOnCutVideoImgListener;
import com.example.ffmpegproject.listener.WlOnErrorListener;
import com.example.ffmpegproject.listener.WlOnGlSurfaceViewOncreateListener;
import com.example.ffmpegproject.listener.WlOnInfoListener;
import com.example.ffmpegproject.listener.WlOnLoadListener;
import com.example.ffmpegproject.listener.WlOnPreparedListener;
import com.example.ffmpegproject.listener.WlOnStopListener;
import com.example.ffmpegproject.listener.WlStatus;
import com.example.ffmpegproject.opengles.MyGLSurfaceView;
import com.example.ffmpegproject.util.MyLog;

import java.nio.ByteBuffer;

/**
 * author:zouguibao
 * date: 2020-04-21
 * desc:
 */
public class VideoPlayer {
    static {
        Log.e("zouguibao","VideoPlayer  static............");
        System.loadLibrary("native-lib");
    }

    public native void hello();
    public native void prepared(String url, boolean isOnlyMusic);

    public native void start();

    public native void pause();

    public native void resume();

    public native void stop(boolean exit);

    public native void seek(int secds);

    public native int getDuration();

    public native void setAudioChannels(int index);

    public native int getVideoWidth();

    public native int getVideoHeight();

    public native int getAudioChannels();


    private String dataSource;

    // 硬解类型
    private MediaFormat mediaFormat;

    // 硬解码器
    private MediaCodec mediaCodec;

    private Surface surface;

    private MyGLSurfaceView myGLSurfaceView;

    // 硬解码器信息
    private MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();

    private WlOnPreparedListener wlOnPreparedListener;

    private WlOnErrorListener wlOnErrorListener;

    private WlOnLoadListener wlOnLoadListener;

    private WlOnStopListener wlOnStopListener;

    /**
     * 更新时间回调
     */
    private WlOnInfoListener wlOnInfoListener;


    private WlOnCompleteListener wlOnCompleteListener;

    private WlOnCutVideoImgListener wlOnCutVideoImgListener;

    private boolean prepared = false;

    private WlTimeBean wlTimeBean;

    private int lastCurrTime = 0;

    private boolean isOnlyMusic = false;

    private boolean isOnlySoft = false;

    public VideoPlayer() {
        wlTimeBean = new WlTimeBean();
    }

    public void setDataSource(String dataSource) {
        this.dataSource = dataSource;
    }

    public void setOnlyMusic(boolean onlyMusic) {
        isOnlyMusic = onlyMusic;
    }

    public void setSurface(Surface surface) {
        this.surface = surface;
    }

    public void setMyGLSurfaceView(MyGLSurfaceView myGLSurfaceView) {
        this.myGLSurfaceView = myGLSurfaceView;
        myGLSurfaceView.setOnGlSurfaceViewOncreateListener(new WlOnGlSurfaceViewOncreateListener() {
            @Override
            public void onGlSurfaceViewOncreate(Surface s) {
                MyLog.d("onGlSurfaceViewOncreate111111111111111");
                if (surface == null) {
                    setSurface(s);
                }
                MyLog.d("onGlSurfaceViewOncreate222222222222222");
                if (prepared && !TextUtils.isDigitsOnly(dataSource)) {
                    prepared(dataSource, isOnlyMusic);
                }
            }

            @Override
            public void onCutVideoImg(Bitmap bitmap) {
                if (wlOnCutVideoImgListener != null) {
                    wlOnCutVideoImgListener.onCutVideoImg(bitmap);
                }
            }
        });
    }

    public void preparedPlay() {
        MyLog.d("preparedPlay11111111.............");
        if (TextUtils.isEmpty(dataSource)) {
            onError(WlStatus.WL_STATUS_DATASOURCE_NULL, "datasource is null");
            return;
        }
        MyLog.d("preparedPlay2222222222.............");
        prepared = true;
        if (isOnlyMusic) {
            prepared(dataSource, isOnlyMusic);
        } else {
            MyLog.d("preparedPlay33333333333.............");
            if (surface != null) {
                MyLog.d("preparedPlay444444444444.............");
                prepared(dataSource, isOnlyMusic);
            }
        }
    }

    public void startPlay() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                if (TextUtils.isEmpty(dataSource)) {
                    onError(WlStatus.WL_STATUS_DATASOURCE_NULL, "datasource is null");
                    return;
                }

                if (!isOnlyMusic) {
                    if (surface == null) {
                        onError(WlStatus.WL_STATUS_SURFACE_NULL, "surface is null");
                        return;
                    }
                }
                if (wlTimeBean == null) {
                    wlTimeBean = new WlTimeBean();
                }
                start();
            }
        }).start();
    }

    public void pausePlay() {
        pause();
    }

    public void resumePlay() {
        resume();
    }

    public void seekPlay(final int secds) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                seek(secds);
                lastCurrTime = 0;
            }
        }).start();
    }

    private void onLoadPlay(boolean load) {
        if (wlOnLoadListener != null) {
            wlOnLoadListener.onLoad(load);
        }
    }

    private void onErrorPlay(int code, String msg) {
        if (wlOnErrorListener != null) {
            wlOnErrorListener.onError(code, msg);
        }
        stop(true);
    }

    private void onParpared() {
        if (wlOnPreparedListener != null) {
            wlOnPreparedListener.onPrepared();
        }
    }


    public void stopPlay(final boolean exit) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                stop(exit);
                if (mediaCodec != null) {
                    mediaCodec.flush();
                    mediaCodec.stop();
                    mediaCodec.release();
                    mediaCodec = null;
                    mediaFormat = null;
                }

                if (myGLSurfaceView != null) {
                    myGLSurfaceView.setCodecType(-1);
                    myGLSurfaceView.requestRender();
                }
            }
        }).start();
    }

    public void setWlOnPreparedListener(WlOnPreparedListener wlOnPreparedListener) {
        this.wlOnPreparedListener = wlOnPreparedListener;
    }


    public void setWlOnErrorListener(WlOnErrorListener wlOnErrorListener) {
        this.wlOnErrorListener = wlOnErrorListener;
    }

    public void setOnlySoft(boolean soft) {
        this.isOnlySoft = soft;
    }

    public boolean isOnlySoft() {
        return isOnlySoft;
    }

    private void onLoad(boolean load) {
        if (wlOnLoadListener != null) {
            wlOnLoadListener.onLoad(load);
        }
    }

    private void onError(int code, String msg) {
        if (wlOnErrorListener != null) {
            wlOnErrorListener.onError(code, msg);
        }
        stopPlay(true);
    }

    public void setWlOnInfoListener(WlOnInfoListener wlOnInfoListener) {
        this.wlOnInfoListener = wlOnInfoListener;
    }

    public void setWlOnCutVideoImgListener(WlOnCutVideoImgListener wlOnCutVideoImgListener) {
        this.wlOnCutVideoImgListener = wlOnCutVideoImgListener;
    }

    public void setWlOnLoadListener(WlOnLoadListener wlOnLoadListener) {
        this.wlOnLoadListener = wlOnLoadListener;
    }

    public void setWlOnCompleteListener(WlOnCompleteListener wlOnCompleteListener) {
        this.wlOnCompleteListener = wlOnCompleteListener;
    }

    public void setWlOnStopListener(WlOnStopListener wlOnStopListener) {
        this.wlOnStopListener = wlOnStopListener;
    }

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


    public void setFrameData(int w, int h, byte[] y, byte[] u, byte[] v) {
        if (myGLSurfaceView != null) {
            MyLog.d("setFrameData");
            myGLSurfaceView.setCodecType(0);
            myGLSurfaceView.setFrameData(w, h, y, u, v);
        }
    }


    public void setVideoInfo(int currt_secd, int total_secd) {
        if (wlOnInfoListener != null && wlTimeBean != null) {
            if (currt_secd < lastCurrTime) {
                currt_secd = lastCurrTime;
            }
            wlTimeBean.setCurrt_secds(currt_secd);
            wlTimeBean.setTotal_secds(total_secd);
            wlOnInfoListener.onInfo(wlTimeBean);
            lastCurrTime = currt_secd;
        }
    }

    public void videoComplete() {
        if (wlOnCompleteListener != null) {
            setVideoInfo(getDuration(), getDuration());
            wlTimeBean = null;
            wlOnCompleteListener.onComplete();
        }
    }

    public void cutVideoImg() {
        if (myGLSurfaceView != null) {
            myGLSurfaceView.cutVideoImg();
        }
    }

    public void onStopComplete() {
        if (wlOnStopListener != null) {
            wlOnStopListener.onStop();
        }
    }


    public void mediacodecInit(int mimetype, int width, int height, byte[] csd0, byte[] csd1)
    {
        MyLog.d("videoplayer mediacodecInit");
        if(surface != null)
        {
            try {
                myGLSurfaceView.setCodecType(1);
                String mtype = getMimeType(mimetype);
                mediaFormat = MediaFormat.createVideoFormat(mtype, width, height);
                mediaFormat.setInteger(MediaFormat.KEY_WIDTH, width);
                mediaFormat.setInteger(MediaFormat.KEY_HEIGHT, height);
                mediaFormat.setLong(MediaFormat.KEY_MAX_INPUT_SIZE, width * height);
                mediaFormat.setByteBuffer("csd-0", ByteBuffer.wrap(csd0));
                mediaFormat.setByteBuffer("csd-1", ByteBuffer.wrap(csd1));
                mediaCodec = MediaCodec.createDecoderByType(mtype);
                if(surface != null)
                {
                    mediaCodec.configure(mediaFormat, surface, null, 0);
                    mediaCodec.start();
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        else
        {
            if(wlOnErrorListener != null)
            {
                wlOnErrorListener.onError(WlStatus.WL_STATUS_SURFACE_NULL, "surface is null");
            }
        }
    }

    public void mediacodecDecode(byte[] bytes, int size, int pts)
    {
        if(bytes != null && mediaCodec != null && info != null)
        {
            try
            {
                int inputBufferIndex = mediaCodec.dequeueInputBuffer(10);
                if(inputBufferIndex >= 0)
                {
                    ByteBuffer byteBuffer = mediaCodec.getInputBuffers()[inputBufferIndex];
                    byteBuffer.clear();
                    byteBuffer.put(bytes);
                    mediaCodec.queueInputBuffer(inputBufferIndex, 0, size, pts, 0);
                }
                int index = mediaCodec.dequeueOutputBuffer(info, 10);
                while (index >= 0) {
                    mediaCodec.releaseOutputBuffer(index, true);
                    index = mediaCodec.dequeueOutputBuffer(info, 10);
                }
            }catch (Exception e)
            {
                e.printStackTrace();
            }
        }
    }


}
