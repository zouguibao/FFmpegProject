package com.example.ffmpegproject;

/**
 * author:zouguibao
 * date: 2020-04-16
 * desc:
 */
public interface OnVideoPlayListener {
    public void onPrepared();
    public void onUpdateCurrentPosition();
    public void onCompleted();
}
