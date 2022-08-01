#ifndef _JITTER_BUFFER_H_
#define _JITTER_BUFFER_H_

#include "agoratype.h"
#include <functional>

using videoOutFn_t=std::function<void (const uint8_t* buffer,
                                       const size_t& bufferLength,
                                       const bool& isKeyFrame)>;

using audioOutFn_t=std::function<void (const uint8_t* buffer,
                                       const int& isKeyFrame)>;

//a jitter buffer that sync audio and video 
class SyncBuffer{
public:
  SyncBuffer(const uint16_t& videoDelayOffset=0,
               const uint16_t& audioDelayOffset=0,
               const bool& syncAudioVideo=false);

  //use this function to add a new video frame to JB
  void addVideo(const uint8_t* buffer,
                const size_t& length,
                const int& isKeyFrame,
                const uint64_t& ts);

  //use this function to add a new audio packet to JB
  void addAudio(const uint8_t* buffer,
                const size_t& length,
                const uint64_t& ts);

  //set a video function that will be called by JB when 
  //there is a video frame available 
  void setVideoOutFn(const videoOutFn_t& fn);

  //set an audio function that will be called by JB when 
  //there is an audio packet available 
  void setAudioOutFn(const audioOutFn_t& fn);

  void clear();

  //start running the jitter buffer thread
  void start();

  //stop running the jitter buffer thread
  void stop();

protected:
  void videoThread();
  void audioThread();

  TimePoint getNextSamplingPoint(const WorkQueue_ptr& q, 
                                 const  unsigned long& currentTimestamp,
                                 const  long& lastTimestamp);

  void WaitForBuffering();
  void checkAndFillInVideoJb(const TimePoint& lastSendTime);
  
private:

  WorkQueue_ptr                   _videoBuffer;
  WorkQueue_ptr                   _audioBuffer;

  std::shared_ptr<std::thread>    _videoSendThread;
  std::shared_ptr<std::thread>    _audioSendThread;

  bool                            _isRunning;

  videoOutFn_t                    _videoOutFn;
  audioOutFn_t                    _audioOutFn;

  uint16_t                        _maxBufferSize; //in ms
  bool                            _isJbBuffering;

  uint16_t                        _videoDelayOffset; //in ms
  uint16_t                        _audioDelayOffset; //in ms

  bool                            _syncAudioVideo;

  int                            _objId;
};

#endif
