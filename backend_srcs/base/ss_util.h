//
// Created by lambert on 23-4-16.
//

#pragma once

#include <cstdint>
#include <string>
#include <functional>
#include <unistd.h>
#include <map>
#include <sys/epoll.h>
#include <chrono>
#include <memory>

constexpr int kSocketError = -1;
constexpr int kInvalidSocket = -1;
constexpr int kNetLibOk = 0;
constexpr int kNetLibError = -1;
constexpr int kInvalidHandle = -1;

enum class NetLibMsg{
  CONNECT,
  CONFIRM,
  READ,
  WRITE,
  CLOSE,
  TIMER,
  LOOP
};

namespace util {

//A function to split a string into a list of strings
class CStrExplode {
 public:
  CStrExplode(char *str, char seperator);
  virtual ~CStrExplode();

  uint32_t GetItemCnt() { return item_cnt_; }
  char *GetItem(uint32_t idx) { return item_list_[idx]; }

 private:
  uint32_t item_cnt_;
  char **item_list_;
};
//when using callback_ function, we need to pass the params by binding
using SocketCallback = std::function<void(NetLibMsg, int handle)>;
using TimerCallback = std::function<void()>;
}
