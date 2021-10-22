#ifndef _PCM_FRAME_OBSERVER_H_
#define _PCM_FRAME_OBSERVER_H_

#include <functional>
#include <map>
#include <string>
#include <memory>

#include "AgoraBase.h"

using OnNewAudioFrame_fn=std::function<void(const uint userId, 
                                        const uint8_t* buffer,
                                        const size_t& size)>;

using OnUserSpeakingFn=std::function<void(const std::string& userId, const int& volume)>;

 class AudioUser;
 using AudioUser_ptr=std::shared_ptr<AudioUser>;

class PcmFrameObserver : public agora::media::IAudioFrameObserver {
 public:
  PcmFrameObserver(): _onAudioFrameReceived(nullptr),
                    _onUserSpeaking(nullptr){
    _pcmUsers.clear();
  }

  void onUserJoined(const std::string& userId);
  void onUserLeft(const std::string& userId);

  void setOnUserSpeakingFn(const OnUserSpeakingFn& fn){
     _onUserSpeaking=fn;
  }

  bool onPlaybackAudioFrame(AudioFrame& audioFrame) override;

  bool onRecordAudioFrame(AudioFrame& audioFrame) override { return true; };

  bool onMixedAudioFrame(AudioFrame& audioFrame) override ;

  bool onPlaybackAudioFrameBeforeMixing(unsigned int uid, AudioFrame& audioFrame) override;

  void setOnAudioFrameReceivedFn(const OnNewAudioFrame_fn& fn);

protected:

private:

  OnNewAudioFrame_fn                   _onAudioFrameReceived;

  std::map<std::string, AudioUser_ptr>   _pcmUsers;

  OnUserSpeakingFn                       _onUserSpeaking;
};

using PcmFrameObserver_ptr=std::shared_ptr<PcmFrameObserver>;


#endif


