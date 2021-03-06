//
// Created by zouguibao on 2020-05-02.
//

#include "thread.h"
#include "pthread.h"

extern "C" {
#include <android/log.h>
};
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "zouguibao", __VA_ARGS__)

Thread::Thread() {
    pthread_mutex_init(&mLock, NULL);
    pthread_cond_init(&mCondition, NULL);
}

Thread::~Thread() {
}

void Thread::start() {
    handleRun(NULL);
}

void Thread::startAsync() {
    pthread_create(&mThread, NULL, startThread, this);
}

int Thread::wait() {
    if (!mRunning) {
        LOGE("mRunning is false so return 0");
        return 0;
    }
    void *status;
    int ret = pthread_join(mThread, &status);
    LOGE("pthread_join thread return result is %d ", status);
    return (int) ret;
}

void Thread::stop() {
}

void *Thread::startThread(void *ptr) {
    LOGE("starting thread");
    Thread *thread = (Thread *) ptr;
    thread->mRunning = true;
    thread->handleRun(ptr);
    thread->mRunning = false;
}

void Thread::waitOnNotify() {
    pthread_mutex_lock(&mLock);
    pthread_cond_wait(&mCondition, &mLock);
    pthread_mutex_unlock(&mLock);
}

void Thread::notify() {
    pthread_mutex_lock(&mLock);
    pthread_cond_signal(&mCondition);
    pthread_mutex_unlock(&mLock);
}

void Thread::handleRun(void *ptr) {
}