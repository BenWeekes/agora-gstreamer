#ifndef _PCM_FRAME_OBSERVER_H_
#define _PCM_FRAME_OBSERVER_H_

#include <functional>
#include <map>
#include <string>
#include <memory>

#include "AgoraBase.h"

#if SDK_BUILD_NUM>=190534
#include "NGIAgoraAudioTrack.h"
#endif

using OnNewAudioFrame_fn=std::function<void(const uint userId, 
                                        const uint8_t* buffer,
                                        const size_t& size,
                                        const uint64_t& ts)>;

using OnUserSpeakingFn=std::function<void(const std::string& userId, const int& volume)>;

 class AudioUser;
 using AudioUser_ptr=std::shared_ptr<AudioUser>;

#if SDK_BUILD_NUM>=190534 
class PcmFrameObserver : public agora::media::IAudioFrameObserverBase {
#else
class PcmFrameObserver : public agora::media::IAudioFrameObserver {
#endif
 public:

  PcmFrameObserver();

  void onUserJoined(const std::string& userId);
  void onUserLeft(const std::string& userId);

  void setOnUserSpeakingFn(const OnUserSpeakingFn& fn){
     _onUserSpeaking=fn;
  }

#if SDK_BUILD_NUM>=190534 
  bool onPlaybackAudioFrame(const char* channelId,AudioFrame& audioFrame) override;
  bool onRecordAudioFrame(const char* channelId,AudioFrame& audioFrame) override { return true; };
  bool onMixedAudioFrame(const char* channelId,AudioFrame& audioFrame) override {return true;}
  bool onPlaybackAudioFrameBeforeMixing(const char* channelId, agora::media::base::user_id_t userId, AudioFrame& audioFrame) override;
#else
  bool onPlaybackAudioFrame(AudioFrame& audioFrame) override;
  bool onRecordAudioFrame(AudioFrame& audioFrame) override { return true; };
  bool onMixedAudioFrame(AudioFrame& audioFrame) override {return true;}
  bool onPlaybackAudioFrameBeforeMixing(unsigned int uid, AudioFrame& audioFrame) override;
#endif

#if SDK_BUILD_NUM>=214232  //sdk 3.8202 
  virtual bool onEarMonitoringAudioFrame(AudioFrame& audioFrame){
     return true;
  }
#endif

  void setOnAudioFrameReceivedFn(const OnNewAudioFrame_fn& fn);
  void setUserJoined(const bool& flag);

  int getObservedAudioFramePosition() override{return 0;}

  AudioParams getPlaybackAudioParams() override{
     AudioParams p; 
     return p;
   }
  AudioParams getRecordAudioParams() override{
    AudioParams p; 
    return p;
  }
  AudioParams getMixedAudioParams() override{
    AudioParams p; 
    return p;
  }
  AudioParams getEarMonitoringAudioParams() override{
    AudioParams p; 
    return p;
  }


protected:

private:

  OnNewAudioFrame_fn                   _onAudioFrameReceived;

  std::map<std::string, AudioUser_ptr>   _pcmUsers;

  OnUserSpeakingFn                       _onUserSpeaking;
  bool                                   _isUserJoined;
};

using PcmFrameObserver_ptr=std::shared_ptr<PcmFrameObserver>;


#endif


