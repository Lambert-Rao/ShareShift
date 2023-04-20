//
// Created by lambert on 23-4-18.
//

#include "event_dispatch.h"
using namespace std::chrono;
constexpr int kMinTimerDuration = 100; // 100ms

XEventDispatch::XEventDispatch() {
  running_ = false;
  epfd_ = epoll_create(1024);
    if (epfd_ == -1) {
        printf("epoll_create failed");
    }
}
XEventDispatch::~XEventDispatch() {
  close(epfd_);
}

void XEventDispatch::AddTimer(const util::Callback& callback, milliseconds interval) {
  for (auto it = timer_list_.begin(); it != timer_list_.end(); it++) {
    TimerItem *pItem = *it;
    if (pItem->callback == callback) {
      pItem->interval = interval;
      pItem->next_tick = util::GetTickCount() + interval;
      return;
    }
  }

  TimerItem *pItem = new TimerItem;
  pItem->callback = callback;
  pItem->interval = interval;
  pItem->next_tick = util::GetTickCount() + interval;
  timer_list_.push_back(pItem);
}

void XEventDispatch::RemoveTimer(util::Callback callback) {
  for (auto it = timer_list_.begin(); it != timer_list_.end(); it++) {
    TimerItem *pItem = *it;
    if (pItem->callback == callback) {
      timer_list_.erase(it);
      delete pItem;
      return;
    }
  }
}