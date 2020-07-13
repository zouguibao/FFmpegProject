package com.example.ffmpegproject.record;

import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.ImageView;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.example.ffmpegproject.R;
import com.example.ffmpegproject.record.encoder.MediaAudioEncoder;
import com.example.ffmpegproject.record.encoder.MediaEncoder;
import com.example.ffmpegproject.record.encoder.MediaMuxerWrapper;
import com.example.ffmpegproject.record.encoder.MediaVideoEncoder;
import com.example.ffmpegproject.record.view.AspectGLSurfaceView;
import com.example.ffmpegproject.util.MyLog;

import java.io.IOException;

/**
 * author:zouguibao
 * date: 2020-05-09
 * desc:
 */
public class RecordActivity extends AppCompatActivity {

    private static final boolean DEBUG = true;
    private static final String TAG = "CameraFragment";


    private AspectGLSurfaceView aspectGLSurfaceView;
    private ImageView mRecordButton;
    /**
     * muxer for audio/video recording
     */
    private MediaMuxerWrapper mMuxer;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_opengl_record);
        aspectGLSurfaceView = (AspectGLSurfaceView) findViewById(R.id.camera_textureview);
        mRecordButton = (ImageView) findViewById(R.id.capture_btn);
        mRecordButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mMuxer == null)
                    startRecording();
                else
                    stopRecording();
            }
        });
    }


    @Override
    protected void onResume() {
        super.onResume();
        aspectGLSurfaceView.onResume();
    }


    /**
     * start resorcing
     * This is a sample project and call this on UI thread to avoid being complicated
     * but basically this should be called on private thread because prepareing
     * of encoder is heavy work
     */
    private void startRecording() {
        MyLog.e("startRecording:");
        try {
            mRecordButton.setColorFilter(0xffff0000);    // turn red
            mMuxer = new MediaMuxerWrapper(".mp4");    // if you record audio only, ".m4a" is also OK.
            if (true) {
                // for video capturing
                new MediaVideoEncoder(mMuxer, mMediaEncoderListener, aspectGLSurfaceView.getVideoWidth(), aspectGLSurfaceView.getVideoHeight());
            }
            if (true) {
                // for audio capturing
                new MediaAudioEncoder(mMuxer, mMediaEncoderListener);
            }
            mMuxer.prepare();
            mMuxer.startRecording();
        } catch (final IOException e) {
            mRecordButton.setColorFilter(0);
            Log.e(TAG, "startCapture:", e);
        }
    }


    /**
     * request stop recording
     */
    private void stopRecording() {
        if (DEBUG) Log.v(TAG, "stopRecording:mMuxer=" + mMuxer);
        mRecordButton.setColorFilter(0);    // return to default color
        if (mMuxer != null) {
            mMuxer.stopRecording();
            mMuxer = null;
            // you should not wait here
        }
    }


    /**
     * callback methods from encoder
     */
    private final MediaEncoder.MediaEncoderListener mMediaEncoderListener = new MediaEncoder.MediaEncoderListener() {
        @Override
        public void onPrepared(final MediaEncoder encoder) {
            if (DEBUG) Log.v(TAG, "onPrepared:encoder=" + encoder);
            if (encoder instanceof MediaVideoEncoder)
                aspectGLSurfaceView.setVideoEncoder((MediaVideoEncoder)encoder);
        }

        @Override
        public void onStopped(final MediaEncoder encoder) {
            if (DEBUG) Log.v(TAG, "onStopped:encoder=" + encoder);
            if (encoder instanceof MediaVideoEncoder)
                aspectGLSurfaceView.setVideoEncoder(null);
        }
    };


}
