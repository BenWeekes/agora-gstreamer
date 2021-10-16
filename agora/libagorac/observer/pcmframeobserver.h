#ifndef _PCM_FRAME_OBSERVER_H_
#define _PCM_FRAME_OBSERVER_H_

#include <functional>

#include "AgoraBase.h"

using OnNewAudioFrame_fn=std::function<void(const uint userId, 
                                        const uint8_t* buffer,
                                        const size_t& size)>;

class PcmFrameObserver : public agora::media::IAudioFrameObserver {
 public:
  PcmFrameObserver(): _onAudioFrameReceived(nullptr){}

  bool onPlaybackAudioFrame(AudioFrame& audioFrame) override;

  bool onRecordAudioFrame(AudioFrame& audioFrame) override { return true; };

  bool onMixedAudioFrame(AudioFrame& audioFrame) override ;

  bool onPlaybackAudioFrameBeforeMixing(unsigned int uid, AudioFrame& audioFrame) override;

  int calcVol(const int16_t* newPacket, const uint16_t& packetLen);

  void setOnAudioFrameReceivedFn(const OnNewAudioFrame_fn& fn);

private:

  OnNewAudioFrame_fn                   _onAudioFrameReceived;
};

using PcmFrameObserver_ptr=std::shared_ptr<PcmFrameObserver>;


#endif


