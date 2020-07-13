package com.example.ffmpegproject.record_video.view;

import android.content.Context;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.opengl.Matrix;
import android.util.AttributeSet;


import com.example.ffmpegproject.record.opengl.GLDrawer2D;
import com.example.ffmpegproject.record_video.camera.RecordCamera;
import com.example.ffmpegproject.util.MyLog;

import java.nio.ByteBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * author:zouguibao
 * date: 2020-05-02
 * desc:
 */
public class CameraGLSurfaceView extends GLSurfaceView implements GLSurfaceView.Renderer, SurfaceTexture.OnFrameAvailableListener {
    private static final String TAG = "CameraGLSurfaceView";
    private static final int SCALE_STRETCH_FIT = 0;
    private static final int SCALE_KEEP_ASPECT_VIEWPORT = 1;
    private static final int SCALE_KEEP_ASPECT = 2;
    private static final int SCALE_CROP_CENTER = 3;

    public int mScaleMode = SCALE_STRETCH_FIT;

    private Context mContext;
    private SurfaceTexture surfaceTexture;
    private int mTexttureID = -1;
    //    DirectDrawer mDirectDrawer;
    private RecordCamera mRecordCamera;

    private int mWidth;
    private int mHeight;
    private final float[] mMvpMatrix = new float[16];
    private final float[] mStMatrix = new float[16];
    private GLDrawer2D mGlDrawer2D;

    public CameraGLSurfaceView(Context context) {
        super(context);
        init(context);
    }

    public CameraGLSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context);
    }


    public void init(Context context) {
        mContext = context;
        setEGLContextClientVersion(2);
        setRenderer(this);
        Matrix.setIdentityM(mMvpMatrix, 0);
    }

    @Override
    public void onFrameAvailable(SurfaceTexture surfaceTexture) {
        this.requestRender();
    }

    private int createTextureID() {
        int[] texture = new int[1];
        GLES20.glGenTextures(1, texture, 0);
        // 绑定纹理
        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, texture[0]);
        // 设置放大缩小。设置边缘测量
        GLES20.glTexParameterf(GLES11Ext.GL_TEXTURE_EXTERNAL_OES,
                GL10.GL_TEXTURE_MIN_FILTER, GL10.GL_LINEAR);
        GLES20.glTexParameterf(GLES11Ext.GL_TEXTURE_EXTERNAL_OES,
                GL10.GL_TEXTURE_MAG_FILTER, GL10.GL_LINEAR);
        GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES,
                GL10.GL_TEXTURE_WRAP_S, GL10.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES,
                GL10.GL_TEXTURE_WRAP_T, GL10.GL_CLAMP_TO_EDGE);

        //解除纹理绑定
        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, 0);

        return texture[0];
    }


    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
//        mTexttureID = createTextureID();
        mTexttureID = GLDrawer2D.initTex();
        surfaceTexture = new SurfaceTexture(mTexttureID);
        surfaceTexture.setOnFrameAvailableListener(this);
//        mDirectDrawer = new DirectDrawer(mTexttureID);
        mRecordCamera = new RecordCamera(mContext, surfaceTexture);
        mRecordCamera.openCamera(Camera.CameraInfo.CAMERA_FACING_BACK);
        mRecordCamera.setCameraPreviewTextture();
        mGlDrawer2D = new GLDrawer2D();
        mGlDrawer2D.setMatrix(mMvpMatrix, 0);
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
//        GLES20.glViewport(0, 0, width, height);
        updateViewport(width,height);
        this.mWidth = width;
        this.mHeight = height;
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        GLES20.glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT | GLES20.GL_DEPTH_BUFFER_BIT);
        surfaceTexture.updateTexImage();

//        mDirectDrawer.draw();
        surfaceTexture.getTransformMatrix(mStMatrix);
        mGlDrawer2D.draw(mTexttureID, mStMatrix);

    }


    public SurfaceTexture getSurfaceTexture() {
        return surfaceTexture;
    }

    @Override
    public void onPause() {
        super.onPause();
    }

    @Override
    public void onResume() {
        super.onResume();
    }

    public ByteBuffer getFrameBuffers() {
        //这里传递的长宽。其实就是openGL surface的长款，一定要注意了！！
        ByteBuffer rgbaBuf = ByteBuffer.allocateDirect(mWidth * mHeight * 4);
        rgbaBuf.position(0);
        long start = System.nanoTime();
        GLES20.glReadPixels(0, 0, mWidth, mHeight, GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE,
                rgbaBuf);
        long end = System.nanoTime();
        MyLog.e("gl glReadPixels cost = " + (end - start));
        return rgbaBuf;
    }

    public void release() {
        mRecordCamera.releaseCamera();
    }

    public void updateViewport(int vWidth, int vHeight) {
        final int view_width = vWidth;
        final int view_height = vHeight;
        GLES20.glViewport(0, 0, view_width, view_height);
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
        final double video_width = vWidth;
        final double video_height = vHeight;
        if (video_width == 0 || video_height == 0) return;
        Matrix.setIdentityM(mMvpMatrix, 0);
        final double view_aspect = view_width / (double) view_height;
        MyLog.e(String.format("view(%d,%d)%f,video(%1.0f,%1.0f)", view_width, view_height, view_aspect, video_width, video_height));
        switch (mScaleMode) {
            case SCALE_STRETCH_FIT:
                break;
            case SCALE_KEEP_ASPECT_VIEWPORT: {
                final double req = video_width / video_height;
                int x, y;
                int width, height;
                if (view_aspect > req) {
                    // if view is wider than camera image, calc width of drawing area based on view height
                    y = 0;
                    height = view_height;
                    width = (int) (req * view_height);
                    x = (view_width - width) / 2;
                } else {
                    // if view is higher than camera image, calc height of drawing area based on view width
                    x = 0;
                    width = view_width;
                    height = (int) (view_width / req);
                    y = (view_height - height) / 2;
                }
                // set viewport to draw keeping aspect ration of camera image
                MyLog.e(String.format("xy(%d,%d),size(%d,%d)", x, y, width, height));
                GLES20.glViewport(x, y, width, height);
                break;
            }
            case SCALE_KEEP_ASPECT:
            case SCALE_CROP_CENTER: {
                final double scale_x = view_width / video_width;
                final double scale_y = view_height / video_height;
                final double scale = (mScaleMode == SCALE_CROP_CENTER
                        ? Math.max(scale_x, scale_y) : Math.min(scale_x, scale_y));
                final double width = scale * video_width;
                final double height = scale * video_height;
                MyLog.e(String.format("size(%1.0f,%1.0f),scale(%f,%f),mat(%f,%f)",
                        width, height, scale_x, scale_y, width / view_width, height / view_height));
                Matrix.scaleM(mMvpMatrix, 0, (float) (width / view_width), (float) (height / view_height), 1.0f);
                break;
            }
        }
        if (mGlDrawer2D != null)
            mGlDrawer2D.setMatrix(mMvpMatrix, 0);

    }
}
