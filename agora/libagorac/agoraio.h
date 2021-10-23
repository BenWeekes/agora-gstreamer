#ifndef _AGORA_IO_H_
#define _AGORA_IO_H_

#include <memory>
#include <chrono>
#include <functional>
#include <fstream>
#include <list>

#include "agoratype.h"
#include "helpers/context.h"

#include "IAgoraService.h"
#include "AgoraBase.h"

#include "observer/pcmframeobserver.h"
#include "observer/h264frameobserver.h"
#include "observer/userobserver.h"

class AgoraIo{
  public:

   AgoraIo(const bool& verbose);

   bool  init(char* in_app_id, 
                        char* in_ch_id,
                        char* in_user_id,
                        bool is_audiouser,
                        bool enable_enc,
		                    short enable_dual,
                        unsigned int  dual_vbr, 
			                  unsigned short  dual_width,
                        unsigned short  dual_height,
                        unsigned short min_video_jb,
                        unsigned short dfps);

    int sendVideo(const uint8_t * buffer,  
                        unsigned long len,
                        int is_key_frame,
                        long timestamp);

    void setOnAudioFrameReceivedFn(const OnNewAudioFrame_fn& fn);
    void setOnVideoFrameReceivedFn(const OnNewFrame_fn& fn);

    size_t getNextVideoFrame(uint8_t* data, size_t max_buffer_size, int* is_key_frame);
    size_t getNextAudioFrame(uint8_t* data, size_t max_buffer_size);

    void addAudioFrame(const Work_ptr& work);

protected:

  bool doConnect(const std::string& appid);

  agora::base::IAgoraService* createAndInitAgoraService(bool enableAudioDevice,
                                                      bool enableAudioProcessor,
						                                          bool enableVideo,
						                                          bool stringUserid,
						                                          bool enableEncryption,
                                                      const char* appid);

  bool doSendHighVideo(const uint8_t* buffer,
                       uint64_t len,int is_key_frame);

  bool doSendAudio( const uint8_t* buffer,  uint64_t len);

  void UpdatePredictedFps(const long& timestamp);

   void VideoThreadHandlerHigh();
   void AudioThreadHandler();
 
   void agora_disconnect();

   //receiver events
   void subscribeToVideoUser(const std::string& userId);

   void receiveVideoFrame(const uint userId, 
                           const uint8_t* buffer,
                           const size_t& length,
                           const int& isKeyFrame);

   void receiveAudioFrame(const uint userId, 
                           const uint8_t* buffer,
                           const size_t& length);

   void handleUserStateChange(const std::string& userId, 
                              const UserState& newState);

    void subscribeAudioUser(const std::string& userId);
    void unsubscribeAudioUser(const std::string& userId);

 private:

    WorkQueue_ptr                                 _receivedVideoFrames;
    WorkQueue_ptr                                 _receivedAudioFrames;

    bool                                          _verbose;

    TimePoint                                     _lastReceivedFrameTime;

    std::list<std::string>                         _activeUsers;
    std::string                                    _currentVideoUser;

    agora::base::IAgoraService*                     _service;
    agora::agora_refptr<agora::rtc::IRtcConnection> _connection;
    agora::rtc::RtcConnectionConfiguration          _rtcConfig;

    bool                                           _connected = false;

    std::shared_ptr<H264FrameReceiver>   h264FrameReceiver;

    PcmFrameObserver_ptr                 _pcmFrameObserver;
    ConnectionObserver_ptr               _connectionObserver;
    UserObserver_ptr                     _userObserver;

    agora::agora_refptr<agora::rtc::IMediaNodeFactory> _factory;

    agora::agora_refptr<agora::rtc::ILocalAudioTrack> _customAudioTrack;
    agora::agora_refptr<agora::rtc::ILocalVideoTrack> _customVideoTrack;

    agora::agora_refptr<agora::rtc::IVideoEncodedImageSender> _videoFrameSender;
    agora::agora_refptr<agora::rtc::IAudioEncodedFrameSender>  _audioSender;

    std::shared_ptr<std::thread>                  _videoThreadHigh;
    std::shared_ptr<std::thread>                    _videoThreadLow;

    std::shared_ptr<std::thread>                    _audioThread;

    WorkQueue_ptr                                   _videoJB;
    WorkQueue_ptr                                   _audioJB;

    TimePoint                                       _lastVideoUserSwitchTime;

    bool                                            _isRunning;

 };

#endif
