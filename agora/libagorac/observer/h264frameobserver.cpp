#include "h264frameobserver.h"
#include <iostream>

//H264FrameReceiver
H264FrameReceiver::H264FrameReceiver()
{
}

bool H264FrameReceiver::OnEncodedVideoImageReceived(const uint8_t* imageBuffer, size_t length, 
    const agora::rtc::EncodedVideoFrameInfo& videoEncodedFrameInfo)
{
    if (!_onVideoFrameReceived)
        return false;


    bool isKeyFrame=videoEncodedFrameInfo.frameType == agora::rtc::VIDEO_FRAME_TYPE_KEY_FRAME;
    
    _onVideoFrameReceived(videoEncodedFrameInfo.uid,
                          imageBuffer, length,isKeyFrame,
#if SDK_BUILD_NUM>=190534 
                          videoEncodedFrameInfo.captureTimeMs
#else
                          videoEncodedFrameInfo.renderTimeMs
#endif
                          );

    return true;
}

void H264FrameReceiver::setOnVideoFrameReceivedFn(const OnNewFrame_fn& fn){
   _onVideoFrameReceived=fn;
}
