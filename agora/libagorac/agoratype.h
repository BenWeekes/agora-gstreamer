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

enum AgoraEventType{

   AGORA_EVENT_ON_IFRAME=1,
   AGORA_EVENT_ON_CONNECTING,
   AGORA_EVENT_ON_CONNECTED,
   AGORA_EVENT_ON_DISCONNECTED,

   AGORA_EVENT_ON_USER_STATE_CHANED,

   AGORA_EVENT_ON_UPLINK_NETWORK_INFO_UPDATED,
   AGORA_EVENT_ON_CONNECTION_LOST,
   AGORA_EVENT_ON_CONNECTION_FAILURE,

   AGORA_EVENT_ON_RECONNECTING,
   AGORA_EVENT_ON_RECONNECTED,

   AGORA_EVENT_ON_VIDEO_SUBSCRIBED

};

enum State{

   USER_STATE_JOIN=1,
   USER_STATE_LEAVE,
   USER_STATE_CAM_ON,
   USER_STATE_CAM_OFF,
   USER_STATE_MIC_ON,
   USER_STATE_MIC_OFF
};

typedef void (*event_fn)(void* userData, 
                         int type, 
                         const char* userName,
                         long param1,
                         long param2);

#endif 
