#ifndef _H264_FRAME_OBSERVER_H_
#define _H264_FRAME_OBSERVER_H_

#include <functional>
#include "AgoraBase.h"

using OnNewFrame_fn=std::function<void(const uint userId, 
                                        const uint8_t* buffer,
                                        const size_t& size,
                                        const int isKeyFrame)>;

class H264FrameReceiver : public agora::rtc::IVideoEncodedImageReceiver
{
public:
    H264FrameReceiver();

    bool OnEncodedVideoImageReceived(const uint8_t* imageBuffer, size_t length, 
        const agora::rtc::EncodedVideoFrameInfo& videoEncodedFrameInfo) override;

    void setOnVideoFrameReceivedFn(const OnNewFrame_fn& fn);

private:
    OnNewFrame_fn                   _onVideoFrameReceived;
};

#endif
