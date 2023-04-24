//
// Created by lambert on 23-4-21.
//

#pragma once

#include <chrono>
#include <functional>
#include <mutex>
#include <future>
#include <utility>
#include <queue>

/**
 * to use the thread pool, you should call Init(n) first
 * then call Start() to start the thread pool
 * after that, you can call Exec() to execute a task
 * if you need to wait for all done, call WaitAllDone()
 * At last, call Stop() to stop the thread pool
 */



class ThreadPool {
 protected:
  struct Task {
    Task(std::chrono::milliseconds interval, std::function<void()> task)
        : interval_(interval), task_(std::move(task)) {
      time_point_ = std::chrono::steady_clock::now() + interval_;
    }
    explicit Task(std::chrono::milliseconds interval)
        : interval_(interval) {
      time_point_ = std::chrono::steady_clock::now() + interval_;
    }
    std::function<void()> task_;
    std::chrono::steady_clock::time_point time_point_;
    std::chrono::milliseconds interval_;
  };
  using TaskPtr = std::unique_ptr<Task>;

  std::queue<TaskPtr> tasks_;
  std::vector<std::thread> threads_;
  std::mutex mutex_;
  std::condition_variable work_cond_;
  //cond_var used when TODO ?
  size_t thread_num_;
  std::atomic<bool> terminate_;
  std::atomic<int> atomic_{0};

 public:
  ThreadPool();
  virtual ~ThreadPool();
  bool Init(size_t worker_num);
  //num of all threads
  size_t GetThreadNum();
  //num of working threads
  size_t GetJobNum();
  void Stop();
  bool Start();


  template<typename Func, typename... Args>
  auto Exec(Func &&f, Args &&...args) -> std::future<decltype(f(args...))> {
    return Exec(std::chrono::milliseconds(0), std::forward<Func>(f),
                std::forward<Args>(args)...);
  }
  /*
   * Exec() do not do the work right now
   */
  template<typename Func, typename... Args>
  auto Exec(std::chrono::milliseconds timeout, Func &&f, Args &&...args)
  -> std::future<decltype(f(args...))> {
    using RetType = decltype(f(args...));
    auto task = std::make_unique<std::packaged_task<RetType()>>(
        std::bind(std::forward<Func>(f), std::forward<Args>(args)...));
    TaskPtr task_ptr = std::make_unique<Task>(timeout, [task]() { (*task)(); });

    std::lock_guard<std::mutex> lock(mutex_);
    tasks_.push(task_ptr);
    work_cond_.notify_one();//wake up one thread to do the new task
    return task->get_future();
  }


  bool WaitAllDone(std::chrono::milliseconds timeout = std::chrono::milliseconds(-1));



 protected:
  bool GetTask(TaskPtr &task);//get a task from task queue
  void Run();
  bool IsTerminate() const { return terminate_.load(); }
};