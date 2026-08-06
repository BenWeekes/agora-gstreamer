#ifndef _YUV_FRAME_OBSERVER_H_
#define _YUV_FRAME_OBSERVER_H_

#include <functional>
#include "AgoraBase.h"
#if SDK_BUILD_NUM>=190534
#include "NGIAgoraLocalUser.h"
#endif

using OnNewDecodedFrame_fn=std::function<void(const char* channelId, agora::user_id_t remoteUid, const agora::media::base::VideoFrame* videoFrame)>;

class YuvFrameObserver : public agora::rtc::IVideoFrameObserver2{
 public:
  YuvFrameObserver(){}
  void onFrame(const char* channelId, agora::user_id_t remoteUid, const agora::media::base::VideoFrame* frame) override;
  virtual ~YuvFrameObserver() = default;
  void setOnVideoFrameReceivedFn(const OnNewDecodedFrame_fn& fn);

private:
  OnNewDecodedFrame_fn                   _onVideoFrameReceived;
};

using YuvFrameObserver_ptr=std::shared_ptr<YuvFrameObserver>;
#endif

