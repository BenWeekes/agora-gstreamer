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

PcmFrameObserver::PcmFrameObserver():
 _onAudioFrameReceived(nullptr),
_onUserSpeaking(nullptr),
_isUserJoined(false){

    _pcmUsers.clear();

}

#if SDK_BUILD_NUM>=190534
 bool PcmFrameObserver::onPlaybackAudioFrameBeforeMixing(const char* channelId, agora::media::base::user_id_t userId, AudioFrame& audioFrame) {
#else
 bool PcmFrameObserver::onPlaybackAudioFrameBeforeMixing(unsigned int userId, AudioFrame& audioFrame) {
#endif

#if SDK_BUILD_NUM>=190534 
  auto userIdString=std::string(userId);
#else
   auto userIdString=std::to_string(userId);
#endif

  auto iter=_pcmUsers.find(userIdString);

  if(iter!=_pcmUsers.end()){

      auto user=iter->second;
      auto isSpeaking=user->onAudioPacket((int16_t*)audioFrame.buffer, audioFrame.samplesPerChannel);
      if(isSpeaking && _onUserSpeaking!=nullptr){
         _onUserSpeaking(userIdString, user->lastVolume());
          //std::cout<<"user: "<<uid<<" is speaking, volume: "<<user->lastVolume()<<std::endl;
      }
  }

  return true;
}

#if SDK_BUILD_NUM>=190534 
 bool PcmFrameObserver::onPlaybackAudioFrame(const char* channelId,AudioFrame& audioFrame){
#else
 bool PcmFrameObserver::onPlaybackAudioFrame(AudioFrame& audioFrame){ 
#endif
   if(_onAudioFrameReceived!=nullptr && _isUserJoined){

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

void PcmFrameObserver::setUserJoined(const bool& flag){
   _isUserJoined=flag;
}
