#include "yuvframeobserver.h"
void YuvFrameObserver::onFrame(const char* channelId, agora::user_id_t remoteUid, const agora::media::base::VideoFrame* videoFrame) {
    if (!_onVideoFrameReceived)
        return;
     _onVideoFrameReceived(channelId,remoteUid,videoFrame);
};

void YuvFrameObserver::setOnVideoFrameReceivedFn(const OnNewDecodedFrame_fn& fn){
   _onVideoFrameReceived=fn;
}