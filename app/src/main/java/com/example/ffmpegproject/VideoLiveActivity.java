package com.example.ffmpegproject;

import android.Manifest;
import android.annotation.SuppressLint;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.os.Build;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.os.Bundle;
import android.view.View;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

import com.example.ffmpegproject.bean.WlTimeBean;
import com.example.ffmpegproject.listener.WlOnCompleteListener;
import com.example.ffmpegproject.listener.WlOnCutVideoImgListener;
import com.example.ffmpegproject.listener.WlOnErrorListener;
import com.example.ffmpegproject.listener.WlOnInfoListener;
import com.example.ffmpegproject.listener.WlOnLoadListener;
import com.example.ffmpegproject.listener.WlOnPreparedListener;
import com.example.ffmpegproject.opengles.MyGLSurfaceView;
import com.example.ffmpegproject.util.MyLog;
import com.example.ffmpegproject.util.TimeUtil;
import com.example.ffmpegproject.video.VideoPlayer;


public class VideoLiveActivity extends AppCompatActivity {

    private MyGLSurfaceView surfaceview;
    private VideoPlayer wlPlayer;
    private ProgressBar progressBar;
    private TextView tvTime;
    private ImageView ivPause;
    private SeekBar seekBar;
    private String pathurl;
    private LinearLayout lyAction;
    private ImageView ivCutImg;
    private ImageView ivShowImg;
    private boolean ispause = false;
    private int position;
    private boolean isSeek = false;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {//5.0 全透明实现
            getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                    | View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION);
            getWindow().setStatusBarColor(Color.TRANSPARENT);
            getWindow().setNavigationBarColor(Color.TRANSPARENT);
        }
        //透明状态栏
        else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {//4.4全透明
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_NAVIGATION);
        }

        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        setContentView(R.layout.activity_main1);
        surfaceview = findViewById(R.id.surfaceview);
        progressBar = findViewById(R.id.pb_loading);
        ivPause = findViewById(R.id.iv_pause);
        tvTime = findViewById(R.id.tv_time);
        seekBar = findViewById(R.id.seekbar);
        lyAction = findViewById(R.id.ly_action);
        ivCutImg = findViewById(R.id.iv_cutimg);
        ivShowImg = findViewById(R.id.iv_show_img);

        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.READ_EXTERNAL_STORAGE}, 1);
        }

        wlPlayer = new VideoPlayer();
        wlPlayer.setOnlyMusic(false);
        wlPlayer.hello();

//        pathurl = getIntent().getExtras().getString("url");
        pathurl = Environment.getExternalStorageDirectory() + "/aa.mp4";

        seekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                position = wlPlayer.getDuration() * progress / 100;
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                isSeek = true;
                MyLog.d("onStopTrackingTouch position = " + position);
                wlPlayer.seekPlay(position);
            }
        });
        wlPlayer.setDataSource(pathurl);
        wlPlayer.setOnlySoft(false);
        wlPlayer.setMyGLSurfaceView(surfaceview);
        wlPlayer.setWlOnErrorListener(new WlOnErrorListener() {
            @Override
            public void onError(int code, String msg) {
                MyLog.d("code:" + code + ",msg:" + msg);
                Message message = Message.obtain();
                message.obj = msg;
                message.what = 3;
                handler.sendMessage(message);
            }
        });

        wlPlayer.setWlOnPreparedListener(new WlOnPreparedListener() {
            @Override
            public void onPrepared() {
                MyLog.d("starting......");
                wlPlayer.startPlay();
            }
        });

        wlPlayer.setWlOnLoadListener(new WlOnLoadListener() {
            @Override
            public void onLoad(boolean load) {
                MyLog.d("loading ......>>>   " + load);
                Message message = Message.obtain();
                message.what = 1;
                message.obj = load;
                handler.sendMessage(message);
            }
        });

        wlPlayer.setWlOnInfoListener(new WlOnInfoListener() {
            @Override
            public void onInfo(WlTimeBean wlTimeBean) {
                Message message = Message.obtain();
                message.what = 2;
                message.obj = wlTimeBean;
                MyLog.d("nowTime is " + wlTimeBean.getCurrt_secds());
                handler.sendMessage(message);
            }
        });

        wlPlayer.setWlOnCompleteListener(new WlOnCompleteListener() {
            @Override
            public void onComplete() {
                MyLog.d("complete .....................");
                wlPlayer.stopPlay(true);
            }
        });

        wlPlayer.setWlOnCutVideoImgListener(new WlOnCutVideoImgListener() {
            @Override
            public void onCutVideoImg(Bitmap bitmap) {
                Message message = Message.obtain();
                message.what = 4;
                message.obj = bitmap;
                handler.sendMessage(message);
            }
        });

        wlPlayer.preparedPlay();
    }


    @SuppressLint("HandlerLeak")
    Handler handler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            if (msg.what == 1) {
                boolean load = (boolean) msg.obj;
                if (load) {
                    progressBar.setVisibility(View.VISIBLE);
                } else {
                    if (lyAction.getVisibility() != View.VISIBLE) {
                        lyAction.setVisibility(View.VISIBLE);
                        ivCutImg.setVisibility(View.VISIBLE);
                    }
                    progressBar.setVisibility(View.GONE);
                }
            } else if (msg.what == 2) {
                WlTimeBean wlTimeBean = (WlTimeBean) msg.obj;
                if (wlTimeBean.getTotal_secds() > 0) {
                    seekBar.setVisibility(View.VISIBLE);
                    if (isSeek) {
                        seekBar.setProgress(position * 100 / wlTimeBean.getTotal_secds());
                        isSeek = false;
                    } else {
                        seekBar.setProgress(wlTimeBean.getCurrt_secds() * 100 / wlTimeBean.getTotal_secds());
                    }
                    tvTime.setText(TimeUtil.secdsToDateFormat(wlTimeBean.getTotal_secds()) + "/" + TimeUtil.secdsToDateFormat(wlTimeBean.getCurrt_secds()));
                } else {
                    seekBar.setVisibility(View.GONE);
                    tvTime.setText(TimeUtil.secdsToDateFormat(wlTimeBean.getCurrt_secds()));
                }
            } else if (msg.what == 3) {
                String err = (String) msg.obj;
                Toast.makeText(VideoLiveActivity.this, err, Toast.LENGTH_SHORT).show();
            } else if (msg.what == 4) {
                Bitmap bitmap = (Bitmap) msg.obj;
                if (bitmap != null) {
                    ivShowImg.setVisibility(View.VISIBLE);
                    ivShowImg.setImageBitmap(bitmap);
                }
            }
        }
    };

    @Override
    public void onBackPressed() {
        if (wlPlayer != null) {
            wlPlayer.stop(true);
        }
        this.finish();
    }

    public void pause(View view) {
        if (wlPlayer != null) {
            ispause = !ispause;
            if (ispause) {
                wlPlayer.pausePlay();
                ivPause.setImageResource(R.drawable.ic_play_play);
            } else {
                wlPlayer.resumePlay();
                ivPause.setImageResource(R.drawable.ic_play_pause);
            }
        }

    }

    public void cutImg(View view) {
        if (wlPlayer != null) {
            wlPlayer.cutVideoImg();
        }
    }


    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (grantResults[0] == PackageManager.PERMISSION_GRANTED) {
            Toast.makeText(getApplicationContext(), "获取存储权限成功", Toast.LENGTH_SHORT).show();
        }
    }
}
