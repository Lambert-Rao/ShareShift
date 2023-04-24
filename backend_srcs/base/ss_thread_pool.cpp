//
// Created by lambert on 23-4-21.
//

#include "ss_thread_pool.h"
ThreadPool::ThreadPool() {
    thread_num_ = 1;
    terminate_ = false;
}

ThreadPool::~ThreadPool() {
    Stop();
}

bool ThreadPool::Init(size_t worker_num) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!threads_.empty()) {
        return false;
    }
    thread_num_ = worker_num;
    return true;
}

void ThreadPool::Stop() {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        terminate_ = true;
        work_cond_.notify_all();
    }
    for (auto &thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
        delete &thread;
    }
    std::unique_lock<std::mutex> lock(mutex_);
    threads_.clear();
}
