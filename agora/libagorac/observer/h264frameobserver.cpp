#include "h264frameobserver.h"
#include <iostream>
#include <fstream>

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

    //for testing received video, we may save that o a file
    /*std::ofstream file("internal.h264",std::ios::app|std::ios::binary);
    if(file.is_open()){
      file.write(reinterpret_cast<const char*>(&imageBuffer[0]), length);  
    }
    std::cout<<"received: "<<length<<" bytes"<<std::endl;*/
  
    
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
