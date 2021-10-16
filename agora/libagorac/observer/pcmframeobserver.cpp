#include "pcmframeobserver.h"

bool PcmFrameObserver::onPlaybackAudioFrameBeforeMixing(unsigned int uid, AudioFrame& audioFrame) {
   
   if(_onAudioFrameReceived!=nullptr){

        _onAudioFrameReceived(uid, (const unsigned char*)audioFrame.buffer, audioFrame.samplesPerChannel*2);        
   }
  
  return true;
}
