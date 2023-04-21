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
      pItem->next_tick = steady_clock::now() + interval;
      return;
    }
  }

  TimerItem *pItem = new TimerItem;
  pItem->callback = callback;
  pItem->interval = interval;
  pItem->next_tick = steady_clock::now() + interval;
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

void XEventDispatch::CheckTimer() {
  auto now = steady_clock::now();
  for (auto it = timer_list_.begin(); it != timer_list_.end();) {
    TimerItem *pItem = *it;
    it++;//move the iterator first, because the callback may remove the timer
    if (now >= pItem->next_tick) {
      pItem->callback();
      pItem->next_tick = now + pItem->interval;
    }
  }
}

void XEventDispatch::AddLoop(util::Callback callback) {
  auto pItem = new TimerItem;
    pItem->callback = callback;
    loop_list_.push_back(pItem);
}

void XEventDispatch::CheckLoop() {
    for (auto it = loop_list_.begin(); it != loop_list_.end(); it++) {
        TimerItem *pItem = *it;
        pItem->callback();
    }
}
XEventDispatch *XEventDispatch::Instance() {
  if(event_dispatch_ == nullptr) {
    event_dispatch_ = new XEventDispatch();
  }
    return event_dispatch_;
}

void XEventDispatch::AddEvent(int fd) {
  struct epoll_event ev{};
  ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLPRI | EPOLLERR | EPOLLHUP;
    ev.data.fd = fd;
    if(epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
        printf("epoll_ctl failed");
    }
}

void XEventDispatch::RemoveEvent(int fd) {
  if(epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr) == -1) {
        printf("epoll_ctl failed");
    }
}

void XEventDispatch::StartDispatch(uint32_t wait_timeout) {
  struct epoll_event events[1024];
  int nfds =0;
  if (running_) {
    return;
  }
    running_ = true;

  while(running_){
    nfds= epoll_wait(epfd_, events, 1024, wait_timeout);
    for(int i=0; i<nfds; i++) {
      int ev_fd = events[i].data.fd;
      XBaseSocket *pSocket = XSocketManager::Instance()->FindBaseSocket(ev_fd);
        if(!pSocket)
            continue;
        if(events[i].events & EPOLLIN) {
            pSocket->OnRead();
        }
        if(events[i].events & EPOLLOUT) {
            pSocket->OnWrite();
        }
        if(events[i].events & EPOLLPRI| EPOLLERR | EPOLLHUP) {
            pSocket->OnClose();
        }

        //TODO pSocket should be a shared_ptr
        delete pSocket;
    }
    //Everytime we check it to see if there is any timer to be triggered
    CheckTimer();
    CheckLoop();
  }
}

void XEventDispatch::StopDispatch() {
  running_ = false;
}