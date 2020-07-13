package com.example.ffmpegproject;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Environment;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

import com.example.ffmpegproject.myvideo.MyVideoPlayer;
import com.example.ffmpegproject.opengles.MyGLSurfaceView;
import com.example.ffmpegproject.util.MyLog;

/**
 * author:zouguibao
 * date: 2020-04-23
 * desc:
 */
public class MyVideoActivity extends AppCompatActivity {

    private String videoPath;
    private MyVideoPlayer player;

    private SurfaceView surfaceView;
    private MyGLSurfaceView myGLSurfaceView;
    private SurfaceHolder surfaceHolder;
    private TextView currentTimeView, totalTimeView;
    private SeekBar seekBar;
    private Button playBtn;
    private Button pauseBtn;
    private Button stopBtn;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main2);
        videoPath = Environment.getExternalStorageDirectory() + "/aa.mp4";
        surfaceView = (SurfaceView) findViewById(R.id.surface_view);
        surfaceHolder = surfaceView.getHolder();

//        myGLSurfaceView = (MyGLSurfaceView) findViewById(R.id.my_surface_view);
//
//        player = new MyVideoPlayer(myGLSurfaceView.getHolder().getSurface());
//        player.setSoftCodec(false);

        currentTimeView = (TextView) findViewById(R.id.current_time_view);
        totalTimeView = (TextView) findViewById(R.id.total_time_view);
        seekBar = (SeekBar) findViewById(R.id.seek_bar);
        playBtn = (Button) findViewById(R.id.play_btn);
        pauseBtn = (Button) findViewById(R.id.pause_btn);
        stopBtn = (Button) findViewById(R.id.stop_btn);
        playBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (player.isPause()) {
                    MyLog.d("playBtn resume.............");
                    player.resume();
                } else {
                    MyLog.d("playBtn startPlay.............");
                    startPlay();
                }
            }
        });
        pauseBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                player.pause();
            }
        });
        stopBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                player.stop();
                seekBar.setProgress(0);
                currentTimeView.setText(formatTime(0));
            }
        });
        seekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {

            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                MyLog.d("seek progress = " + seekBar.getProgress());
                player.seekTo(seekBar.getProgress());
            }
        });

        surfaceHolder.addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                MyLog.d("surfaceCreated................");
                player = new MyVideoPlayer(holder.getSurface());
                player.setSoftCodec(true);
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {

            }
        });

        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.READ_EXTERNAL_STORAGE}, 1);
        }
    }

    public void playVideo(View view) {
        player.playVideo(videoPath, surfaceHolder.getSurface());
    }

    public void playAudio(View view) {
        player.playAudio(videoPath);
    }


    private void startPlay() {
        player.play(videoPath, surfaceHolder.getSurface(), new MyVideoPlayer.PlayerCallback() {
            @Override
            public void onStart() {
                MyLog.d("播放开始了 -------------------");
            }

            @Override
            public void onProgress(final int total, final int current) {
//                if (current == 0) {
//                    MyLog.d("total = " + total + "   current = " + current);
//                }
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        currentTimeView.setText(formatTime(current));
                        totalTimeView.setText(formatTime(total));
                        seekBar.setMax(total);
                        seekBar.setProgress(current);
                    }
                });
            }

            @Override
            public void onEnd() {
                MyLog.d("播放结束了 -------------------");
            }
        });
    }

    private String formatTime(int time) {
        int minute = time / 60;
        int second = time % 60;
        return (minute < 10 ? ("0" + minute) : minute) + ":" + (second < 10 ? ("0" + second) : second);
    }

    public void seekTo(View view) {
        player.seekTo(5);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        player.detroyMediaCodec();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (grantResults[0] == PackageManager.PERMISSION_GRANTED) {
            Toast.makeText(getApplicationContext(), "获取存储权限成功", Toast.LENGTH_SHORT).show();
        }
    }
}
