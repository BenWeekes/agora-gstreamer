#ifndef _AGORA_TYPE_H_
#define _AGORA_TYPE_H_

#include <string.h>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <memory>

#include "workqueue.h"

class AgoraDecoder;
using AgoraDecoder_ptr=std::shared_ptr<AgoraDecoder>;

class AgoraEncoder;
using AgoraEncoder_ptr=std::shared_ptr<AgoraEncoder>;

class LocalConfig;
using LocalConfig_ptr=std::shared_ptr<LocalConfig>;

using TimePoint=std::chrono::steady_clock::time_point;

typedef  struct agora_context_t agora_context_t;
typedef  void (*agora_log_func_t)(void*, const char*);

#endif 
