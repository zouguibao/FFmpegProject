package com.example.ffmpegproject.record.view;

import android.graphics.SurfaceTexture;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.opengl.Matrix;

import com.example.ffmpegproject.record.encoder.MediaVideoEncoder;
import com.example.ffmpegproject.record.opengl.GLDrawer2D;
import com.example.ffmpegproject.util.MyLog;

import java.lang.ref.WeakReference;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * author:zouguibao
 * date: 2020-05-09
 * desc:
 */
class CameraSurfaceRenderer implements GLSurfaceView.Renderer, SurfaceTexture.OnFrameAvailableListener {
    public static final int SCALE_STRETCH_FIT = 0;
    public static final int SCALE_KEEP_ASPECT_VIEWPORT = 1;
    public static final int SCALE_KEEP_ASPECT = 2;
    public static final int SCALE_CROP_CENTER = 3;
    private WeakReference<AspectGLSurfaceView> mWeakParent;
    public SurfaceTexture mSTexture;    // API >= 11
    public int hTex;
    private GLDrawer2D mDrawer;
    private final float[] mStMatrix = new float[16];
    private final float[] mMvpMatrix = new float[16];
    public MediaVideoEncoder mVideoEncoder;
    private volatile boolean requesrUpdateTex = false;
    private boolean flip = true;

    public CameraSurfaceRenderer(AspectGLSurfaceView aspectGLSurfaceView) {
        mWeakParent = new WeakReference<AspectGLSurfaceView>(aspectGLSurfaceView);
        Matrix.setIdentityM(mMvpMatrix, 0);
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        MyLog.e("onSurfaceCreated............");
        final String extensions = GLES20.glGetString(GLES20.GL_EXTENSIONS);	// API >= 8
//			if (DEBUG) Log.i(TAG, "onSurfaceCreated:Gl extensions: " + extensions);
        if (!extensions.contains("OES_EGL_image_external"))
            throw new RuntimeException("This system does not support OES_EGL_image_external.");
        // create textur ID
        hTex = GLDrawer2D.initTex();

        // create SurfaceTexture with texture ID.
        mSTexture = new SurfaceTexture(hTex);

        mSTexture.setOnFrameAvailableListener(this);

        // clear screen with yellow color so that you can see rendering rectangle
        GLES20.glClearColor(1.0f, 1.0f, 0.0f, 1.0f);

        final AspectGLSurfaceView parent = mWeakParent.get();
        if (parent != null) {
            parent.mHasSurface = true;
        }
        // create object for preview display
        mDrawer = new GLDrawer2D();
        mDrawer.setMatrix(mMvpMatrix, 0);
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        MyLog.e("onSurfaceChanged............");
        updateViewport();
        final AspectGLSurfaceView parent = mWeakParent.get();
        if (parent != null) {
            parent.startPreview(width, height);
        }
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);

        if (requesrUpdateTex) {
            requesrUpdateTex = false;
            if(mSTexture != null){
                // update texture(came from camera)
                mSTexture.updateTexImage();
                // get texture matrix
                mSTexture.getTransformMatrix(mStMatrix);
            }
        }
        // draw to preview screen
        mDrawer.draw(hTex, mStMatrix);
        flip = !flip;
        if (flip) {	// ~30fps
            synchronized (this) {
                if (mVideoEncoder != null) {
                    // notify to capturing thread that the camera frame is available.
//						mVideoEncoder.frameAvailableSoon(mStMatrix);
                    mVideoEncoder.frameAvailableSoon(mStMatrix, mMvpMatrix);
                }
            }
        }

    }



    @Override
    public void onFrameAvailable(SurfaceTexture surfaceTexture) {
        requesrUpdateTex = true;
        final AspectGLSurfaceView parent = mWeakParent.get();
        if (parent != null)
            parent.requestRender();
    }

    public  void updateViewport(){
        final AspectGLSurfaceView parent = mWeakParent.get();
        if (parent != null) {
            final int view_width = parent.getWidth();
            final int view_height = parent.getHeight();
            GLES20.glViewport(0, 0, view_width, view_height);
            GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
            final double video_width = parent.mVideoWidth;
            final double video_height = parent.mVideoHeight;
            if (video_width == 0 || video_height == 0) return;
            Matrix.setIdentityM(mMvpMatrix, 0);
            final double view_aspect = view_width / (double)view_height;
            MyLog.e(String.format("view(%d,%d)%f,video(%1.0f,%1.0f)", view_width, view_height, view_aspect, video_width, video_height));
            switch (parent.mScaleMode) {
                case SCALE_STRETCH_FIT:
                    break;
                case SCALE_KEEP_ASPECT_VIEWPORT:
                {
                    final double req = video_width / video_height;
                    int x, y;
                    int width, height;
                    if (view_aspect > req) {
                        // if view is wider than camera image, calc width of drawing area based on view height
                        y = 0;
                        height = view_height;
                        width = (int)(req * view_height);
                        x = (view_width - width) / 2;
                    } else {
                        // if view is higher than camera image, calc height of drawing area based on view width
                        x = 0;
                        width = view_width;
                        height = (int)(view_width / req);
                        y = (view_height - height) / 2;
                    }
                    // set viewport to draw keeping aspect ration of camera image
                    MyLog.e(String.format("xy(%d,%d),size(%d,%d)", x, y, width, height));
                    GLES20.glViewport(x, y, width, height);
                    break;
                }
                case SCALE_KEEP_ASPECT:
                case SCALE_CROP_CENTER:
                {
                    final double scale_x = view_width / video_width;
                    final double scale_y = view_height / video_height;
                    final double scale = (parent.mScaleMode == SCALE_CROP_CENTER
                            ? Math.max(scale_x,  scale_y) : Math.min(scale_x, scale_y));
                    final double width = scale * video_width;
                    final double height = scale * video_height;
                    MyLog.e(String.format("size(%1.0f,%1.0f),scale(%f,%f),mat(%f,%f)",
                            width, height, scale_x, scale_y, width / view_width, height / view_height));
                    Matrix.scaleM(mMvpMatrix, 0, (float)(width / view_width), (float)(height / view_height), 1.0f);
                    break;
                }
            }
            if (mDrawer != null)
                mDrawer.setMatrix(mMvpMatrix, 0);
        }

    }


    /**
     * when GLSurface context is soon destroyed
     */
    public void onSurfaceDestroyed() {
        MyLog.e("onSurfaceDestroyed:");
        if (mDrawer != null) {
            mDrawer.release();
            mDrawer = null;
        }
        if (mSTexture != null) {
            mSTexture.release();
            mSTexture = null;
        }
        GLDrawer2D.deleteTex(hTex);
    }
}
