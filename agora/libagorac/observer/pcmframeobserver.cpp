#include "pcmframeobserver.h"
#include <iostream>

class AudioUser{
  public:
     AudioUser():
       _isSpeaking(false), 
      _lastVolume(0),
      _maxVolume(0),
      _volExceedThresholdCount(0),
      _volBelowThresholdCount(0){}

     bool onAudioPacket(const int16_t* samples, const uint16_t& packetLen){

        const int _silenceCountThreashold=10;
        const int _speakingCountThreashold=6;

        const int volumeThreshold=2000;

        auto volume=calcVol(samples, packetLen);
        _lastVolume=volume;

        if(_maxVolume<volume){
             _maxVolume=volume;
        }
        if (volume>volumeThreshold){  
           _volExceedThresholdCount++;
           _volBelowThresholdCount=0;
        }
        else {
           _volExceedThresholdCount=0;
           _volBelowThresholdCount++;
        }

        if(_volExceedThresholdCount>_speakingCountThreashold && !_isSpeaking){
           _isSpeaking=true;
        }

        if(_volBelowThresholdCount>_silenceCountThreashold && _isSpeaking==true){
            _isSpeaking=false;
        }

        return _isSpeaking;
     }

     bool isSpeaking() {return _isSpeaking;}
     int  lastVolume() {return _lastVolume;}

protected:

    int calcVol(const int16_t* samples, const uint16_t& packetLen){
	
       int32_t sum=0;
       for(int i=0;i<packetLen;i++)	
         sum +=std::abs(samples[i]);
	
       return sum/(double)packetLen;
      }

  private:
     bool   _isSpeaking;
     int    _lastVolume;

     int _maxVolume;

     int _volExceedThresholdCount;
     int _volBelowThresholdCount;
};

bool PcmFrameObserver::onPlaybackAudioFrameBeforeMixing(unsigned int uid, AudioFrame& audioFrame) {
   
  auto iter=_pcmUsers.find(std::to_string(uid));
  if(iter!=_pcmUsers.end()){
      auto user=iter->second;
      auto isSpeaking=user->onAudioPacket((int16_t*)audioFrame.buffer, audioFrame.samplesPerChannel);
      if(isSpeaking && _onUserSpeaking!=nullptr){
         _onUserSpeaking(std::to_string(uid), user->lastVolume());
          //std::cout<<"user: "<<uid<<" is speaking, volume: "<<user->lastVolume()<<std::endl;
      }
  }

  return true;
}

bool PcmFrameObserver::onMixedAudioFrame(AudioFrame& audioFrame){
  
   return true;
}

 bool PcmFrameObserver::onPlaybackAudioFrame(AudioFrame& audioFrame){

   if(_onAudioFrameReceived!=nullptr){

        //std::cout<<"AUDIO timestampe: "<<audioFrame.renderTimeMs<<std::endl;
        _onAudioFrameReceived(0, 
                              (const unsigned char*)audioFrame.buffer,
                               audioFrame.samplesPerChannel*2,
                               audioFrame.renderTimeMs);        
    }
  
  return true;

 }

 void PcmFrameObserver::setOnAudioFrameReceivedFn(const OnNewAudioFrame_fn& fn){
   _onAudioFrameReceived=fn;
}

void PcmFrameObserver::onUserJoined(const std::string& userId){

  AudioUser_ptr newUser=std::make_shared<AudioUser>();
  _pcmUsers.emplace(userId, newUser);

}
void PcmFrameObserver::onUserLeft(const std::string& userId){

   auto iter=_pcmUsers.find(userId);
   if(iter!=_pcmUsers.end()){
      _pcmUsers.erase(iter);
   }

}
