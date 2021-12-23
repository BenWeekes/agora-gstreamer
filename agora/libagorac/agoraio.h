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
   AgoraIo(const bool& verbose, 
           event_fn fn,
			  void* userData,
           const int& in_audio_delay,
           const int& in_video_delay,
           const int& out_audio_delay,
           const int& out_video_delay);

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
                        uint64_t len,
                        int is_key_frame,
                        long timestamp);

   int sendAudio(const uint8_t * buffer,  
                        uint64_t len,
                        long timestamp);

    void setOnAudioFrameReceivedFn(const OnNewAudioFrame_fn& fn);
    void setOnVideoFrameReceivedFn(const OnNewFrame_fn& fn);

    size_t getNextVideoFrame(uint8_t* data, 
                             size_t max_buffer_size,
                             int* is_key_frame,
                             uint64_t* ts);
                             
    size_t getNextAudioFrame(uint8_t* data, size_t max_buffer_size);

   void disconnect();

   void setPaused(const bool& flag);

   //right now we support two params to the event
   void addEvent(const AgoraEventType& eventType, 
                  const std::string& userName,
                  const long& param1=0, 
                  const long& param2=0,
                  long* states=nullptr);

   void setEventFunction(event_fn fn, void* userData);

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

   //receiver events
   void subscribeToVideoUser(const std::string& userId);

   void receiveVideoFrame(const uint userId, 
                           const uint8_t* buffer,
                           const size_t& length,
                           const int& isKeyFrame,
                           const uint64_t& ts);

   void receiveAudioFrame(const uint userId, 
                           const uint8_t* buffer,
                           const size_t& length,
                           const uint64_t& ts);

   void handleUserStateChange(const std::string& userId, 
                              const UserState& newState);

    void subscribeAudioUser(const std::string& userId);
    void unsubscribeAudioUser(const std::string& userId);

    void unsubscribeAllVideo();


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


    TimePoint                                       _lastVideoUserSwitchTime;

    bool                                            _isRunning;

    TimePoint                                       _lastVideoSendTime;

    bool                                             _isPaused;

    event_fn                                         _eventfn;
    void*                                            _userEventData;

    //from the app to agora sdk
    SyncBuffer_ptr                                   _outSyncBuffer;

    int                                              _in_audio_delay;
    int                                              _in_video_delay;

    int                                              _out_audio_delay;
    int                                              _out_video_delay;

 };

#endif