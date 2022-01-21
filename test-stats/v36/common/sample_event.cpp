//  Agora RTC/MEDIA SDK
//
//  Created by Pengfei Han in 2019-08.
//  Copyright (c) 2019 Agora.io. All rights reserved.
//

#include "sample_event.h"

inline uint64_t tick_ms() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

void SampleEvent::Set() {
  std::lock_guard<std::mutex> _(lock_);
  signal_ = true;
  cv_.notify_one();
}

int SampleEvent::Wait(int wait_ms) {
  std::unique_lock<std::mutex> _(lock_);
  int64_t expired_time = tick_ms() + wait_ms;
  // prevent spurious wakeups from doing harm
  while (!signal_) {
    if (wait_ms < 0) {
      cv_.wait(_);
      continue;
    }
    auto ret = cv_.wait_for(_, std::chrono::milliseconds(wait_ms));
    if (signal_) {
      continue;
    }

    if (ret == std::cv_status::timeout) {
      return -1;
    }
    // false wakeup
    if (tick_ms() > expired_time) {
      return -1;
    }
  }
  // here is the auto reset
  signal_ = false;
  return 0;
}
