// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef _ASYNC_LOGGING_H_
#define _ASYNC_LOGGING_H_

#include "blocking_queue.h"
#include "log_stream.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

class AsyncLogging {
  public:
    AsyncLogging(const string &basename, off_t rollSize, int flushInterval = 3);

    ~AsyncLogging() {
        if (running_) {
            stop();
        }
    }

    void append(const char *logline, int len);

    void start() {
        running_ = true;
        thread_ = new std::thread(&AsyncLogging::threadFunc, this);
    }

    void stop() {
        running_ = false;
        cond_.notify_all();
        thread_->join();
    }

  private:
    void threadFunc();

    typedef detail::FixedBuffer<detail::kLargeBuffer> Buffer;
    typedef std::vector<std::unique_ptr<Buffer>> BufferVector;
    typedef BufferVector::value_type BufferPtr;

    const int flushInterval_;
    bool running_;
    const string basename_;
    const off_t rollSize_;
    std::thread *thread_;

    BufferPtr currentBuffer_;
    BufferPtr nextBuffer_;
    BufferVector buffers_;

    std::mutex mutex_;
    std::condition_variable cond_;
};

#endif // _ASYNC_LOGGING_H_
