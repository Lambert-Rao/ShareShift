//
// Created by lambert on 23-4-21.
//

#pragma once

#include <chrono>
#include <functional>
#include <mutex>

class TreadPool {
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
    return Exec()
  }

 protected:
  struct Task {
    std::function<void()> task_;
    std::chrono::steady_clock::time_point time_point_;
    std::chrono::milliseconds interval_;
  };

};