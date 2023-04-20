//
// Created by lambert on 23-4-18.
//

#pragma once

#include <mutex>
#include <list>

#include "ss_util.h"

enum {
  SOCKET_READ = 0x1,
  SOCKET_WRITE = 0x2,
  SOCKET_EXCEP = 0x4,
  SOCKET_ALL = 0x7
};

class XEventDispatch {

 public:
  virtual ~XEventDispatch();

  void AddEvent(int fd, uint8_t socket_event);
  void RemoveEvent(int fd, uint8_t socket_event);

  //TODO 检查addtimer的调用
  void AddTimer(const util::Callback& callback, std::chrono::milliseconds  interval);
  void RemoveTimer(util::Callback callback);

  void AddLoop(util::Callback callback);

  void StartDispatch(uint32_t wait_timeout = 100);
  void StopDispatch();

  bool IsRunning() { return running_; }

  static XEventDispatch *Instance();
 protected:
  XEventDispatch();
 private:
  struct TimerItem {
    //TODO callback是不是得先绑定？
    util::Callback callback;
    std::chrono::milliseconds interval;
    std::chrono::steady_clock::time_point next_tick;
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