//
// Created by lambert on 23-4-16.
//

#include <cstring>
#include "ss_util.h"


util::CStrExplode::CStrExplode(char *str, char seperator) {
    item_cnt_ = 1;
    char *pos = str;
    while (*pos) {
        if (*pos == seperator) {
            item_cnt_++;
        }

        pos++;
    }

    item_list_ = new char *[item_cnt_];

    int idx = 0;
    char *start = pos = str;
    while (*pos) {
        if (pos != start && *pos == seperator) {
            uint32_t len = pos - start;
            item_list_[idx] = new char[len + 1];
            strncpy(item_list_[idx], start, len);
            item_list_[idx][len] = '\0';
            idx++;

            start = pos + 1;
        }

        pos++;
    }

    uint32_t len = pos - start;
    if (len != 0) {
        item_list_[idx] = new char[len + 1];
        strncpy(item_list_[idx], start, len);
        item_list_[idx][len] = '\0';
    }
}

util::CStrExplode::~CStrExplode() {
    for (uint32_t i = 0; i < item_cnt_; i++) {
        delete[] item_list_[i];
    }

    delete[] item_list_;
}