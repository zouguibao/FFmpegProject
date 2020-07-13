package com.example.ffmpegproject.record.camera;

import android.app.Activity;
import android.content.Context;
import android.graphics.ImageFormat;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.view.Surface;

import com.example.ffmpegproject.util.MyLog;

import java.util.List;

/**
 * author:zouguibao
 * date: 2020-05-01
 * desc:
 */
public class RecordCamera {

    private static final String TAG = "RecordCamera";

    public static int VIDEO_WIDTH = 640;
    public static int DEFAULT_VIDEO_WIDTH = 640;
    public static int VIDEO_HEIGHT = 480;
    public static int DEFAULT_VIDEO_HEIGHT = 480;
    public static int videoFrameRate = 24;


    public static void forcePreviewSize_640_480() {
        VIDEO_WIDTH = 640;
        VIDEO_HEIGHT = 480;
        videoFrameRate = 15;
    }

    public static void forcePreviewSize_1280_720() {
        VIDEO_WIDTH = 1280;
        VIDEO_HEIGHT = 720;
        videoFrameRate = 24;
    }

    private Camera mCamera;
    private SurfaceTexture mSurfaceTexture;
    private Context mContext;

    public RecordCamera(Context mContext, SurfaceTexture surfaceTexture) {
        this.mContext = mContext;
        this.mSurfaceTexture = surfaceTexture;
    }




    public void setCameraPreviewTextture() {
        if (mSurfaceTexture == null) {
            return;
        }
        try {
            mCamera.setPreviewTexture(mSurfaceTexture);
            mSurfaceTexture.setOnFrameAvailableListener(new SurfaceTexture.OnFrameAvailableListener() {
                @Override
                public void onFrameAvailable(SurfaceTexture surfaceTexture) {
                    if (mCallback != null) {
                        mCallback.notifyFrameAvailable();
                    }
                }
            });
            // 开始预览
            mCamera.startPreview();
            MyLog.e("setCameraPreviewTextture startPreview = ");
        } catch (Exception e) {
            e.printStackTrace();
        }
    }



    public int getNumberOfCameras() {
        return Camera.getNumberOfCameras();
    }

    public void releaseCamera() {
        try {

            if (mSurfaceTexture != null) {
                mSurfaceTexture.release();
                mSurfaceTexture = null;
            }

            if (mCamera != null) {
                mCamera.setPreviewCallback(null);
                mCamera.release();
                mCamera = null;
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void openCamera(final int facingId) {
        forcePreviewSize_1280_720();
        try {
            // 1.开启Camera
            mCamera = getCameraInstance(facingId);
            MyLog.e("opencamera。。。11111111111");
        } catch (Exception e) {
            return;
        }
        Camera.Parameters parameters = mCamera.getParameters();
        // 2. 设置预览照片的图像格式
        List<Integer> supportedPreviewFormats = parameters.getSupportedPreviewFormats();

        if (supportedPreviewFormats.contains(ImageFormat.NV21)) {
            parameters.setPreviewFormat(ImageFormat.NV21);
        }

        // 3、设置预览照片的尺寸
        List<Camera.Size> supportedPreviewSizes = parameters.getSupportedPreviewSizes();
        int previewWidth = VIDEO_WIDTH;
        int previewHeight = VIDEO_HEIGHT;
        boolean isSupportPreviewSize = isSupportPreviewSize(supportedPreviewSizes, previewWidth, previewHeight);

        if (isSupportPreviewSize) {
            parameters.setPreviewSize(previewWidth, previewHeight);
        } else {
            previewWidth = DEFAULT_VIDEO_WIDTH;
            previewHeight = DEFAULT_VIDEO_HEIGHT;
            isSupportPreviewSize = isSupportPreviewSize(
                    supportedPreviewSizes, previewWidth, previewHeight);
            if (isSupportPreviewSize) {
                VIDEO_WIDTH = DEFAULT_VIDEO_WIDTH;
                VIDEO_HEIGHT = DEFAULT_VIDEO_HEIGHT;
                parameters.setPreviewSize(previewWidth, previewHeight);
            }
        }
        MyLog.e("opencamera。。。previewWidth = " + previewWidth + " previewHeight = " + previewHeight);

        //下面这行设置 有可能导致 返回的图像尺寸和预期不一致
//        parameters.setRecordingHint(true);

        // 4、设置视频记录的连续自动对焦模式
        if (parameters.getSupportedFocusModes().contains(Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO)) {
            parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO);
        }

        //缩短Recording启动时间
        parameters.setRecordingHint(true);
        //是否支持影像稳定能力，支持则开启
        if (parameters.isVideoStabilizationSupported()) {
            parameters.setVideoStabilization(true);
        }


        try {
            mCamera.setParameters(parameters);
        } catch (Exception e) {

        }

        int degress = getCameraDisplayOrientation((Activity) mContext, facingId);
        mCamera.setDisplayOrientation(degress);
    }


    private boolean isSupportPreviewSize(List<Camera.Size> supportedPreviewSizes, int previewWidth, int previewHeight) {
        boolean isSupportPreviewSize = false;
        for (Camera.Size size : supportedPreviewSizes) {
            if (previewWidth == size.width && previewHeight == size.height) {
                isSupportPreviewSize = true;
                break;
            }
        }
        return isSupportPreviewSize;
    }

    private Camera getCameraInstance(int id) {
        Camera c = null;
        try {
            c = Camera.open(id);
        } catch (Exception e) {
            return null;
        }
        return c;
    }

    public static int getCameraFacing(final int facingId) {
        int result;
        Camera.CameraInfo info = new Camera.CameraInfo();
        Camera.getCameraInfo(facingId, info);
        if (info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
            result = 1;
        } else {
            result = 0;
        }
        return result;
    }

    public static int getCameraDisplayOrientation(final Activity activity, final int facingId) {
        int rotation = activity.getWindowManager().getDefaultDisplay().getRotation();
        int degrees = 0;
        switch (rotation) {
            case Surface.ROTATION_0:
                degrees = 0;
                break;
            case Surface.ROTATION_90:
                degrees = 90;
                break;
            case Surface.ROTATION_180:
                degrees = 180;
                break;
            case Surface.ROTATION_270:
                degrees = 270;
                break;
        }
        int result;
        Camera.CameraInfo info = new Camera.CameraInfo();
        Camera.getCameraInfo(facingId, info);
        if (info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
            result = (info.orientation + degrees) % 360;
        } else { // back-facing
            result = (info.orientation - degrees + 360) % 360;
        }
        return result;
    }

    public interface RecordCameraCallback {
        public void onPermissionDismiss(String tip);

        public void notifyFrameAvailable();
    }

    public RecordCameraCallback mCallback;

    public void setCallback(RecordCameraCallback mCallback) {
        this.mCallback = mCallback;
    }
}
