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
class JitterBuffer{
public:
  JitterBuffer();

  void addVideo(const uint8_t* buffer,
                const size_t& length,
                const int& isKeyFrame,
                const uint64_t& ts);

  void addAudio(const uint8_t* buffer,
                const size_t& length,
                const uint64_t& ts);

  void setVideoOutFn(const videoOutFn_t& fn);
  void setAudioOutFn(const audioOutFn_t& fn);

  //start running the jitter buffer thread
  void start();

  //stop running the jitter buffer thread
  void stop();

protected:
  void videoThread();
  void audioThread();

  TimePoint getNextSamplingPoint(const WorkQueue_ptr& q, 
                                 const long& currentTimestamp,
                                 const long& lastTimestamp);

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

};

#endif
