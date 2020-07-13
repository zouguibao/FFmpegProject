package com.example.ffmpegproject.record.opengl;

import android.graphics.SurfaceTexture;
import android.opengl.EGLContext;
import android.opengl.GLES20;
import android.opengl.Matrix;
import android.text.TextUtils;
import android.view.Surface;
import android.view.SurfaceHolder;

import com.example.ffmpegproject.util.MyLog;

/**
 * author:zouguibao
 * date: 2020-05-09
 * desc:
 */
public class RenderHandler implements Runnable {

    private final Object mSync = new Object();
    private EGLContext mShard_context;
    private boolean mIsRecordable;
    private Object mSurface;
    private int mTexId = -1;
    private float[] mMatrix = new float[32];

    private boolean mRequestSetEglContext;
    private boolean mRequestRelease;
    private int mRequestDraw;

    private RenderHandler() {

    }

    public static final RenderHandler getInstance(String name) {
        final RenderHandler renderHandler = new RenderHandler();
        synchronized (renderHandler.mSync) {
            new Thread(renderHandler, !TextUtils.isEmpty(name) ? name : "RenderHandler").start();
            try {
                renderHandler.mSync.wait();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
        return renderHandler;
    }

    public final void setEGLContext(final EGLContext shared_context, final int tex_id, final Object surface, final boolean isRecordable) {
        if (!(surface instanceof Surface) && !(surface instanceof SurfaceTexture) && !(surface instanceof SurfaceHolder))
            throw new RuntimeException("unsupported window type:" + surface);

        synchronized (mSync) {
            if (mRequestRelease) return;
            mShard_context = shared_context;
            mTexId = tex_id;
            mSurface = surface;
            mIsRecordable = isRecordable;
            mRequestSetEglContext = true;
            Matrix.setIdentityM(mMatrix, 0);
            Matrix.setIdentityM(mMatrix, 16);
            mSync.notifyAll();
            try {
                mSync.wait();
            } catch (final InterruptedException e) {
            }
        }
    }


    public final void draw() {
        draw(mTexId, mMatrix, null);
    }

    public final void draw(final int tex_id) {
        draw(tex_id, mMatrix, null);
    }

    public final void draw(final float[] tex_matrix) {
        draw(mTexId, tex_matrix, null);
    }

    public final void draw(final float[] tex_matrix, final float[] mvp_matrix) {
        draw(mTexId, tex_matrix, mvp_matrix);
    }

    public final void draw(final int tex_id, final float[] tex_matrix) {
        draw(tex_id, tex_matrix, null);
    }


    public final void draw(final int tex_id, final float[] tex_matrix, final float[] mvp_matrix) {
        synchronized (mSync) {
            if (mRequestRelease) return;
            mTexId = tex_id;
            if ((tex_matrix != null) && (tex_matrix.length >= 16)) {
                System.arraycopy(tex_matrix, 0, mMatrix, 0, 16);
            } else {
                Matrix.setIdentityM(mMatrix, 0);
            }
            if ((mvp_matrix != null) && (mvp_matrix.length >= 16)) {
                System.arraycopy(mvp_matrix, 0, mMatrix, 16, 16);
            } else {
                Matrix.setIdentityM(mMatrix, 16);
            }
            mRequestDraw++;
            mSync.notifyAll();
/*			try {
				mSync.wait();
			} catch (final InterruptedException e) {
			} */
        }
    }


    public boolean isValid() {
        synchronized (mSync) {
            return !(mSurface instanceof Surface) || ((Surface) mSurface).isValid();
        }
    }

    public final void release() {
        MyLog.e("release:");
        synchronized (mSync) {
            if (mRequestRelease) return;
            mRequestRelease = true;
            mSync.notifyAll();
            try {
                mSync.wait();
            } catch (final InterruptedException e) {
            }
        }
    }


    private EGLBase mEgl;
    private EGLBase.EglSurface mInputSurface;
    private GLDrawer2D mDrawer;

    @Override
    public void run() {
        MyLog.e("RenderHandler thread started:");
        synchronized (mSync) {
            mRequestSetEglContext = mRequestRelease = false;
            mRequestDraw = 0;
            mSync.notifyAll();
        }
        boolean localRequestDraw;
        for (;;) {
            synchronized (mSync) {
                if (mRequestRelease) break;
                if (mRequestSetEglContext) {
                    mRequestSetEglContext = false;
                    internalPrepare();
                }
                localRequestDraw = mRequestDraw > 0;
                if (localRequestDraw) {
                    mRequestDraw--;
//					mSync.notifyAll();
                }
            }
            if (localRequestDraw) {
                if ((mEgl != null) && mTexId >= 0) {
                    mInputSurface.makeCurrent();
                    // clear screen with yellow color so that you can see rendering rectangle
                    GLES20.glClearColor(1.0f, 1.0f, 0.0f, 1.0f);
                    GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
                    mDrawer.setMatrix(mMatrix, 16);
                    mDrawer.draw(mTexId, mMatrix);
                    mInputSurface.swap();
                }
            } else {
                synchronized(mSync) {
                    try {
                        mSync.wait();
                    } catch (final InterruptedException e) {
                        break;
                    }
                }
            }
        }

        synchronized (mSync) {
            mRequestRelease = true;
            internalRelease();
            mSync.notifyAll();
        }
        MyLog.e("RenderHandler thread finished:");
    }

    private final void internalPrepare() {
        MyLog.e("internalPrepare:");
        internalRelease();
        mEgl = new EGLBase(mShard_context, false, mIsRecordable);

        mInputSurface = mEgl.createFromSurface(mSurface);

        mInputSurface.makeCurrent();
        mDrawer = new GLDrawer2D();
        mSurface = null;
        mSync.notifyAll();
    }

    private final void internalRelease() {
        MyLog.e("internalRelease:");
        if (mInputSurface != null) {
            mInputSurface.release();
            mInputSurface = null;
        }
        if (mDrawer != null) {
            mDrawer.release();
            mDrawer = null;
        }
        if (mEgl != null) {
            mEgl.release();
            mEgl = null;
        }
    }


}
