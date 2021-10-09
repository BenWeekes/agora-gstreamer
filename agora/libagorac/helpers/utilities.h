#ifndef _UTILITIES_H_
#define _UTILITIES_H_

#include "../agoratype.h"

TimePoint GetNextSamplingPoint(agora_context_t* ctx,
                                const float& speed,
                                const WorkQueue_ptr& q);

TimePoint GetNextSamplingPoint(const std::string& label,
                               agora_context_t* ctx, 
                               const WorkQueue_ptr& q, 
                               const long& currentTimestamp,
                               const long& lastTimestamp);

long GetTimeDiff(const TimePoint& start, const TimePoint& end);

std::string GetAddressAsString(agora_context_t* ctx);

void WaitForBuffering(agora_context_t* ctx);

void ResizeBuffer(agora_context_t* ctx);

bool isNumber(const std::string& userIdString);

int getVideoSyncBytesPos(const uint8_t* buffer);

TimePoint Now();
 
#endif
