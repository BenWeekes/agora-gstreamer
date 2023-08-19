#pragma once
#include <functional>
#include "AgoraBase.h"
#if SDK_BUILD_NUM>=190534
#include "NGIAgoraLocalUser.h"
#endif

using OnNewDecodedFrame_fn=std::function<void(const char* channelId, agora::user_id_t remoteUid, const agora::media::base::VideoFrame* videoFrame)>;

class YuvFrameObserver : public agora::rtc::IVideoFrameObserver2{
 public:
  YuvFrameObserver(const std::string& outputFilePath){}
  void onFrame(const char* channelId, agora::user_id_t remoteUid, const agora::media::base::VideoFrame* frame) override;
  virtual ~YuvFrameObserver() = default;
  void setOnVideoFrameReceivedFn(const OnNewDecodedFrame_fn& fn);

private:
  OnNewDecodedFrame_fn                   _onVideoFrameReceived;
};


