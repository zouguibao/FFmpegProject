package com.example.ffmpegproject;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

import android.Manifest;
import android.content.pm.PackageManager;
import android.graphics.PixelFormat;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import java.util.Formatter;
import java.util.Locale;

public class MainActivity extends AppCompatActivity implements OnVideoPlayListener {

    private SurfaceView surfaceView;
    private SurfaceHolder surfaceHolder;
    private TextView duration;
    private TextView curPosition;
    private String videoPath = Environment.getExternalStorageDirectory() + "/aa.mp4";
    private SeekBar mSeekBar;
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(@NonNull Message msg) {
            switch (msg.what) {
                case 1:
                    String durationStr = stringForTime((int) getDuration());
                    duration.setText(durationStr);
                    curPosition.setText(stringForTime(getCurrentTime()));
                    mSeekBar.setProgress(0);
                    break;
                case 2:
                    int d = (int) getDuration();
                    int c = getCurrentTime();
                    float cc = (float) c / d;
                    cc = cc * 100;
                    int dd = (int) cc;
//                    Log.e("zouguibao", "cc = " + cc + "  dd = " + dd);
                    curPosition.setText(stringForTime(getCurrentTime()));
                    mSeekBar.setProgress(dd);
//                    mHandler.sendEmptyMessageDelayed(2,500);
                    break;
                case 3:
                    break;
                default:
                    break;
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        // Example of a call to a native method
//        final TextView tv = (TextView) findViewById(R.id.sample_text);
//         tv.setText(stringFromJNI());
//        tv.setText(avcodecinfo());
        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.READ_EXTERNAL_STORAGE}, 1);
        }
        surfaceView = (SurfaceView) findViewById(R.id.surface_view);
        surfaceHolder = surfaceView.getHolder();
        surfaceHolder.setFormat(PixelFormat.RGBA_8888);
        surfaceHolder.addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                Log.e("zouguibao", "surfaceCreated = ");
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                Log.e("zouguibao", "surfaceChanged = ");
            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {

            }
        });
        duration = (TextView) findViewById(R.id.duration);
        curPosition = (TextView) findViewById(R.id.curpos);
        mSeekBar = (SeekBar) findViewById(R.id.seek_bar);

        mSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                // 进度发生改变时会触发
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                // 按住SeekBar时会触发
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                // 放开SeekBar时触发
            }
        });
    }

    public void play(View view) {
//        Log.e("zouguibao", "videoPath = " + videoPath);
//        ffplay(videoPath, surfaceHolder.getSurface());
        new Thread(new Runnable() {
            @Override
            public void run() {
                Log.e("zouguibao", "videoPath = " + videoPath);
                ffplay(videoPath, surfaceHolder.getSurface());
            }
        }).start();
    }

    public void stop(View view) {
        ffStop();
    }

    public void pause(View view) {
        ffPause();
    }

    /* A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();

    public native String avcodecinfo();

    public native void ffplay(String videoPath, Surface surface);

    public native void ffStop();

    public native void ffPause();

    public native long getVideoDuration(String videoPath);

    public native long getDuration();

    public native int getCurrentTime();

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");

    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (grantResults[0] == PackageManager.PERMISSION_GRANTED) {
            Toast.makeText(getApplicationContext(), "获取存储权限成功", Toast.LENGTH_SHORT).show();
        }
    }


    /**
     * 使用ffmpeg命令行给视频添加水印
     * @param srcFile 源文件
     * @param waterMark 水印文件路径
     * @param targetFile 目标文件
     * @return 添加水印后的文件
     */
    public static  String[] addWaterMark(String srcFile, String waterMark, String targetFile){
        String waterMarkCmd = "ffmpeg -i %s -i %s -filter_complex overlay=0:0 %s";
        waterMarkCmd = String.format(waterMarkCmd, srcFile, waterMark, targetFile);
        return waterMarkCmd.split(" ");//以空格分割为字符串数组
    }

    public String stringForTime(int timeMs) {
        if (timeMs > 0 && timeMs < 86400000) {
            int totalSeconds = timeMs / 1000;
            int seconds = totalSeconds % 60;
            int minutes = totalSeconds / 60 % 60;
            int hours = totalSeconds / 3600;
            StringBuilder stringBuilder = new StringBuilder();
            Formatter mFormatter = new Formatter(stringBuilder, Locale.getDefault());
            return hours > 0 ? mFormatter.format("%d:%02d:%02d", hours, minutes, seconds).toString() : mFormatter.format("%02d:%02d", minutes, seconds).toString();
        } else {
            return "00:00";
        }
    }

    @Override
    public void onPrepared() {
        mHandler.sendEmptyMessage(1);

    }

    @Override
    public void onUpdateCurrentPosition() {
//        Log.e("zouguibao", "ffmpeg play onUpdateCurrentPosition...... = " + stringForTime(getCurrentTime()));
        mHandler.sendEmptyMessage(2);
    }

    @Override
    public void onCompleted() {
//        Log.e("zouguibao", "ffmpeg play onCompleted......");
//        isPlayCompleted = true;
//        mHandler.removeCallbacksAndMessages(null);
        mHandler.sendEmptyMessage(3);
    }
}
