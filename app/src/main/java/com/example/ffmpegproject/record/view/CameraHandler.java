package com.example.ffmpegproject.record.view;


import android.os.Handler;
import android.os.Looper;
import android.os.Message;

import androidx.annotation.NonNull;

import com.example.ffmpegproject.util.MyLog;

/**
 * author:zouguibao
 * date: 2020-05-09
 * desc:
 */
class CameraHandler extends Handler {
    private static final int MSG_PREVIEW_START = 1;
    private static final int MSG_PREVIEW_STOP = 2;
    private CameraThread mThread;

    public CameraHandler(CameraThread thread) {
        mThread = thread;
    }

    @Override
    public void handleMessage(@NonNull Message msg) {
        super.handleMessage(msg);

        switch (msg.what) {
            case MSG_PREVIEW_START:
                mThread.startPreview(msg.arg1, msg.arg2);
                break;
            case MSG_PREVIEW_STOP:
                mThread.stopPreview();
                synchronized (this) {
                    notifyAll();
                }
                Looper.myLooper().quit();
                mThread = null;
                break;
            default:
                throw new RuntimeException("unknown message:what=" + msg.what);
        }
    }


    public void startPreview(final int width, final int height) {
        sendMessage(obtainMessage(MSG_PREVIEW_START, width, height));
    }

    /**
     * request to stop camera preview
     *
     * @param needWait need to wait for stopping camera preview
     */
    public void stopPreview(final boolean needWait) {
        synchronized (this) {
            sendEmptyMessage(MSG_PREVIEW_STOP);
            if (needWait && mThread.mIsRunning) {
                try {
                    MyLog.e("wait for terminating of camera thread");
                    wait();
                } catch (final InterruptedException e) {
                }
            }
        }
    }

}
