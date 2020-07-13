package com.example.ffmpegproject.record.view;

import android.content.Context;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.opengl.EGL14;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;

import com.example.ffmpegproject.record.encoder.MediaVideoEncoder;
import com.example.ffmpegproject.util.MyLog;

/**
 * author:zouguibao
 * date: 2020-05-09
 * desc:
 */
public class AspectGLSurfaceView extends GLSurfaceView {

    private static final boolean DEBUG = true;
    private static final String TAG = "CameraGLView";

    private static final int CAMERA_ID = 0;

    private static final int SCALE_STRETCH_FIT = 0;
    private static final int SCALE_KEEP_ASPECT_VIEWPORT = 1;
    private static final int SCALE_KEEP_ASPECT = 2;
    private static final int SCALE_CROP_CENTER = 3;
    public int mRotation;
    public boolean mHasSurface;

    private CameraSurfaceRenderer mRenderer;
    public CameraHandler mCameraHandler = null;
    public int mVideoWidth, mVideoHeight;
    public int mScaleMode = SCALE_STRETCH_FIT;


    public AspectGLSurfaceView(Context context) {
        super(context);
        init(context);
    }

    public AspectGLSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context);
    }

    private void init(Context context) {
        setEGLContextClientVersion(2);    // GLES 2.0, API >= 8
        mRenderer = new CameraSurfaceRenderer(this);
        setRenderer(mRenderer);
		// the frequency of refreshing of camera preview is at most 15 fps
		// and RENDERMODE_WHEN_DIRTY is better to reduce power consumption
		setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);

    }

    @Override
    public void onResume() {
        super.onResume();

        if (mHasSurface) {
            MyLog.e("aspectsurface onResume ...........");
            if (mCameraHandler == null) {
                MyLog.e("aspectsurface onResume already exist");
                startPreview(getWidth(), getHeight());
            }
        }
    }

    @Override
    public void onPause() {
        if (mCameraHandler != null) {
            // just request stop prviewing
            mCameraHandler.stopPreview(false);
        }
        super.onPause();
    }

    public void setScaleMode(final int mode) {
        if (mScaleMode != mode) {
            mScaleMode = mode;
            queueEvent(new Runnable() {
                @Override
                public void run() {
                    mRenderer.updateViewport();
                }
            });
        }
    }

    public void setVideoSize(final int width, final int height) {
        if ((mRotation % 180) == 0) {
            mVideoWidth = width;
            mVideoHeight = height;
        } else {
            mVideoWidth = height;
            mVideoHeight = width;
        }
        queueEvent(new Runnable() {
            @Override
            public void run() {
                mRenderer.updateViewport();
            }
        });
    }


    public int getScaleMode() {
        return mScaleMode;
    }


    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        if (DEBUG) Log.v(TAG, "surfaceDestroyed:");
        if (mCameraHandler != null) {
            // wait for finish previewing here
            // otherwise camera try to display on un-exist Surface and some error will occure
            mCameraHandler.stopPreview(true);
        }
        mCameraHandler = null;
        mHasSurface = false;
        mRenderer.onSurfaceDestroyed();
        super.surfaceDestroyed(holder);
    }


    public int getVideoWidth() {
        return mVideoWidth;
    }

    public int getVideoHeight() {
        return mVideoHeight;
    }

    public SurfaceTexture getSurfaceTexture() {
        if (DEBUG) Log.v(TAG, "getSurfaceTexture:");
        return mRenderer != null ? mRenderer.mSTexture : null;
    }

    public void startPreview(int width, int height) {
        MyLog.e("aspect startPreview............");
        if (mCameraHandler == null) {
            final CameraThread thread = new CameraThread(this, Camera.CameraInfo.CAMERA_FACING_BACK);
            thread.start();
            mCameraHandler = thread.getHandler();
        }
        mCameraHandler.startPreview(1280, 720/*width, height*/);
    }

    public void setVideoEncoder(final MediaVideoEncoder encoder) {
        if (DEBUG) Log.v(TAG, "setVideoEncoder:tex_id=" + mRenderer.hTex + ",encoder=" + encoder);
        queueEvent(new Runnable() {
            @Override
            public void run() {
                synchronized (mRenderer) {
                    if (encoder != null) {
                        encoder.setEglContext(EGL14.eglGetCurrentContext(), mRenderer.hTex);
                    }
                    mRenderer.mVideoEncoder = encoder;
                }
            }
        });
    }
}
