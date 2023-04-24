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

class TreadPool {
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
  using TaskPtr = std::shared_ptr<Task>;

  std::queue<TaskPtr> tasks_;
  std::vector<std::thread> threads_;
  std::mutex mutex_;
  std::condition_variable cond_var_;
  size_t thread_num_;
  bool terminate_;
  std::atomic<int> atomic_{0};

 public:
  TreadPool();
  virtual ~TreadPool();
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
  template<typename Func, typename... Args>
  auto Exec(std::chrono::milliseconds timeout, Func &&f, Args &&...args)
  -> std::future<decltype(f(args...))> {
    using RetType = decltype(f(args...));
    auto task = std::make_shared<std::packaged_task<RetType()>>(
        std::bind(std::forward<Func>(f), std::forward<Args>(args)...));
    TaskPtr task_ptr = std::make_shared<Task>(timeout, [task]() { (*task)(); });

    std::unique_lock<std::mutex> lock(mutex_);
    tasks_.push(task_ptr);
    cond_var_.notify_one();//wake up one thread to do the new task
    return task->get_future();
  }

  bool WaitAllDone(std::chrono::milliseconds timeout = std::chrono::milliseconds(-1));



 protected:
  bool GetTask(TaskPtr &task);//get a task from task queue
  void Run();
  bool IsTterminate() const { return terminate_; }
};