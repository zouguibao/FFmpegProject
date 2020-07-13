//
// Created by zouguibao on 2020-05-02.
//

#ifndef FFMPEGPROJECT_THREAD_H
#define FFMPEGPROJECT_THREAD_H

#include "CommondTools.h"
#include <pthread.h>

class Thread {
public:
    Thread();

    ~Thread();

    void start();

    void startAsync();

    int wait();

    void waitOnNotify();

    void notify();

    virtual void stop();

protected:
    bool mRunning;

    virtual void handleRun(void *ptr);

protected:
    pthread_t mThread;
    pthread_mutex_t mLock;
    pthread_cond_t mCondition;

    static void *startThread(void *ptr);
};

#endif //FFMPEGPROJECT_THREAD_H
