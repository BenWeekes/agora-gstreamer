#include "pcmframeobserver.h"
#include <iostream>

bool PcmFrameObserver::onPlaybackAudioFrameBeforeMixing(unsigned int uid, AudioFrame& audioFrame) {
   
   /*if(_onAudioFrameReceived!=nullptr){

        _onAudioFrameReceived(uid, (const unsigned char*)audioFrame.buffer, audioFrame.samplesPerChannel*2);        
   }*/

  
  return true;
}

bool PcmFrameObserver::onMixedAudioFrame(AudioFrame& audioFrame){
  
   return true;
}

 bool PcmFrameObserver::onPlaybackAudioFrame(AudioFrame& audioFrame){

   if(_onAudioFrameReceived!=nullptr){

        _onAudioFrameReceived(0, (const unsigned char*)audioFrame.buffer, audioFrame.samplesPerChannel*2);        
    }
  
  return true;

 }
