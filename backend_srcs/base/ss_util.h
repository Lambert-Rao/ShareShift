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

constexpr int kSocketError = -1;
constexpr int kInvalidSocket = -1;
constexpr int kNetLibOk = 0;
constexpr int kNetLibError = -1;

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
using Callback = std::function<void()>;
}
