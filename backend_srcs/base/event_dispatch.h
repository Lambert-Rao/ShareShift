//
// Created by lambert on 23-4-18.
//

#pragma once

#include <mutex>
#include <list>

#include "ss_util.h"

class XEventDispatch {

 public:
  virtual ~XEventDispatch();

  void AddEvent(int fd);
  void RemoveEvent(int fd);

  //TODO 检查addtimer的调用
  void AddTimer(const util::SocketCallback& callback, std::chrono::milliseconds  interval);
  void RemoveTimer(util::SocketCallback callback);

  void AddLoop(util::SocketCallback callback);

  void StartDispatch(uint32_t wait_timeout = 100);
  void StopDispatch();

  bool IsRunning() const { return running_; }

  static XEventDispatch *Instance();
 protected:
  XEventDispatch();
 private:
  struct TimerItem {
    //TODO callback是不是得先绑定？
    util::SocketCallback callback_;
    std::chrono::milliseconds interval_;
    std::chrono::steady_clock::time_point next_tick_;
  };

  void CheckTimer();
  void CheckLoop();
  int epfd_;
  std::mutex lock_;
  std::list<TimerItem *> timer_list_; // 定时器
  std::list<TimerItem *> loop_list_;  // 自定义loop
  static XEventDispatch *event_dispatch_;
  bool running_ = false;
};