#include "agoraio.h"

#include <stdbool.h>
#include <fstream>

#include <string>
#include <cstring>
#include <iostream>
#include <fstream>

//agora header files
#include "NGIAgoraRtcConnection.h"


#include "helpers/agoradecoder.h"
#include "helpers/agoraencoder.h"
#include "helpers/agoralog.h"
#include "helpers/localconfig.h"

//#include "userobserver.h"
#include "observer/connectionobserver.h"
#include "helpers/context.h"

#include "helpers/utilities.h"
#include "agoratype.h"
#include "helpers/agoraconstants.h"

#include "agorareceiver.h"
#include "helpers/uidtofile.h"



AgoraIo::AgoraIo(const bool& verbose,
                event_fn fn,
			    void* userData):
 _verbose(verbose),
 _lastReceivedFrameTime(Now()),
 _currentVideoUser(""),
 _lastVideoUserSwitchTime(Now()),
 _isRunning(false),
 _isPaused(true),
 _eventfn(fn),
 _userEventData(userData){

   _activeUsers.clear();
}

//helper function for creating a service
agora::base::IAgoraService* AgoraIo::createAndInitAgoraService(bool enableAudioDevice,
                                                      bool enableAudioProcessor,
						      bool enableVideo,
						      bool stringUserid,
						      bool enableEncryption, const char* appid) {
  auto service = createAgoraService();
  agora::base::AgoraServiceConfiguration scfg;

  scfg.enableAudioProcessor = true;
  scfg.enableAudioDevice = false;
  scfg.enableVideo = true;

  scfg.useStringUid=stringUserid;
  if (enableEncryption) {
    scfg.appId = appid;
  }

  int ret = service->initialize(scfg);
  return (ret == agora::ERR_OK) ? service : nullptr;
}

bool AgoraIo::doConnect(const std::string& appid)
{
    _service = createAgoraService();
    if (!_service)
    {
        logMessage("Error init Agora SDK");
        return false;
    }

    int32_t buildNum = 0;
    getAgoraSdkVersion(&buildNum);
    logMessage("Agora SDK version: {}"+std::to_string(buildNum));

    agora::base::AgoraServiceConfiguration scfg;
    scfg.appId = appid.c_str();
    scfg.enableAudioProcessor = true;
    scfg.enableAudioDevice = false;
    scfg.enableVideo = true;
 

    if (_service->initialize(scfg) != agora::ERR_OK)
    {
        logMessage("Error initialize Agora SDK");
        return false;
    }

    return true;
}

#define ENC_KEY_LENGTH        128
bool  AgoraIo::init(char* in_app_id, 
                        char* in_ch_id,
                        char* in_user_id,
                        bool is_audiouser,
                        bool enable_enc,
		                    short enable_dual,
                        unsigned int  dual_vbr, 
			                  unsigned short  dual_width,
                        unsigned short  dual_height,
                        unsigned short min_video_jb,
                        unsigned short dfps){

    if(!doConnect(in_app_id)){
        return false;
    }

    std::string _userId=in_user_id;
    
    _rtcConfig.clientRoleType = agora::rtc::CLIENT_ROLE_BROADCASTER;

    _rtcConfig.autoSubscribeAudio = false;
    _rtcConfig.autoSubscribeVideo = false;
    _rtcConfig.enableAudioRecordingOrPlayout = false; 
    
    _connection = _service->createRtcConnection(_rtcConfig);
    if (!_connection)
    {
        logMessage("Error creating connection to Agora SDK");
        return false;
    }
    
    _factory = _service->createMediaNodeFactory();
    if (!_factory)
    {
        logMessage("Error creating factory");
        return false;
    }

   _userObserver=std::make_shared<UserObserver>(_connection->getLocalUser(),_verbose);
   
    //register audio observer
    _pcmFrameObserver = std::make_shared<PcmFrameObserver>();
    if (_connection->getLocalUser()->setPlaybackAudioFrameParameters(1, 48000) != 0) {
        logMessage("Agora: Failed to set audio frame parameters!");
        return false;
    }

    if (_connection->getLocalUser()->setPlaybackAudioFrameBeforeMixingParameters(1, 48000) != 0) {
        logMessage("Agora: Failed to set audio frame parameters!");
        return false;
    }

    // Register connection observer to monitor connection event
    _connectionObserver = std::make_shared<ConnectionObserver>(this);
    _connection->registerObserver(_connectionObserver.get());

    _connection->getLocalUser()->registerAudioFrameObserver(_pcmFrameObserver.get());
    auto res = _connection->connect(in_app_id, in_ch_id, in_user_id);
    if (res)
    {
       logMessage("Error connecting to channel");
        return false;
    }

    _videoFrameSender=_factory->createVideoEncodedImageSender();
    if (!_videoFrameSender) {
       std::cout<<"Failed to create video frame sender!"<<std::endl;
       return false;
    }

    //if you want to send_dual_h264,the ccMode must be enabled
     agora::base::SenderOptions option;
     option.ccMode = agora::base::CC_ENABLED;
    // Create video track
    _customVideoTrack=_service->createCustomVideoTrack(_videoFrameSender, option);
    if (!_customVideoTrack) {
         std::cout<<"Failed to create video track!"<<std::endl;
         return false;
     }

     //audio
    // Create audio data sender
     _audioSender = _factory->createAudioEncodedFrameSender();
     if (!_audioSender) {
        return false;
      }

     // Create audio track
     _customAudioTrack =_service->createCustomAudioTrack(_audioSender, agora::base::MIX_DISABLED);
     if (!_customAudioTrack) {
        return false;
     }

    // Publish  video track
    _connection->getLocalUser()->publishVideo(_customVideoTrack);
    _connection->getLocalUser()->publishAudio(_customAudioTrack);


    h264FrameReceiver = std::make_shared<H264FrameReceiver>();
    _userObserver->setVideoEncodedImageReceiver(h264FrameReceiver.get());

    //video
     _receivedVideoFrames=std::make_shared<WorkQueue <Work_ptr> >();
    h264FrameReceiver->setOnVideoFrameReceivedFn([this](const uint userId, 
                                                    const uint8_t* buffer,
                                                    const size_t& length,
                                                    const int& isKeyFrame,
                                                    const uint64_t& ts){

           receiveVideoFrame(userId, buffer, length, isKeyFrame, ts);

    });

   //audio
     _receivedAudioFrames=std::make_shared<WorkQueue <Work_ptr> >();
    _pcmFrameObserver->setOnAudioFrameReceivedFn([this](const uint userId, 
                                                    const uint8_t* buffer,
                                                    const size_t& length){

          receiveAudioFrame(userId, buffer, length);
    });

    //connection observer: handles user join and leave
    _connectionObserver->setOnUserStateChanged([this](const std::string& userId,
                                                      const UserState& newState){
                                             
        handleUserStateChange(userId, newState);

    });

    _userObserver->setOnUserInfofn([this](const std::string& userId, const int& messsage, const int& value){
        if(messsage==1 && value==1){
           handleUserStateChange(userId, USER_CAM_OFF);

           addEvent(AGORA_EVENT_ON_USER_STATE_CHANED,userId,USER_STATE_CAM_OFF,0);
        }
        else if(messsage==1 && value==0){
            handleUserStateChange(userId, USER_CAM_ON);
            addEvent(AGORA_EVENT_ON_USER_STATE_CHANED,userId,USER_STATE_CAM_ON,0);
        }
        else if(messsage==0 && value==1){
            addEvent(AGORA_EVENT_ON_USER_STATE_CHANED,userId,USER_STATE_MIC_ON,0);
        }
        else if(messsage==0 && value==0){
            addEvent(AGORA_EVENT_ON_USER_STATE_CHANED,userId,USER_STATE_MIC_OFF,0);
        }

    });

    _userObserver->setOnIframeRequestFn([this](){
        addEvent(AGORA_EVENT_ON_IFRAME,"",0,0);
    });

    _userObserver->setOnUserRemoteTrackStatsFn([this](const std::string& userId,
                                                      long* stats){

        addEvent(AGORA_EVENT_ON_REMOTE_TRACK_STATS_CHANGED,userId,0,0,stats);
     });

    _userObserver->setOnUserLocalTrackStatsFn([this](const std::string& userId,
                                                      long* stats){

        addEvent(AGORA_EVENT_ON_LOCAL_TRACK_STATS_CHANGED,userId,0,0,stats);
     });

    _pcmFrameObserver->setOnUserSpeakingFn([this](const std::string& userId, const int& volume){

        //no switching is needed if current user is already shown 
        if(userId==_currentVideoUser){
           return;  
        }

        //no switching if last switch time was less than 3 second
        auto diffTime=GetTimeDiff(_lastVideoUserSwitchTime, Now());
        if(diffTime<3000){
            return;
        }

        if(_currentVideoUser!=""){
            _connection->getLocalUser()->unsubscribeVideo(_currentVideoUser.c_str());
        }
        subscribeToVideoUser(userId);
        _lastVideoUserSwitchTime=Now();
    });

  _videoJB=std::make_shared<WorkQueue <Work_ptr> >();
  _audioJB=std::make_shared<WorkQueue <Work_ptr> >();

  _isRunning=true;

    //start thread handlers
  _videoThreadHigh=std::make_shared<std::thread>(&AgoraIo::VideoThreadHandlerHigh, this);
  _audioThread=std::make_shared<std::thread>(&AgoraIo::AudioThreadHandler,this);

  _connected = true;

  return _connected;
}

void AgoraIo::addAudioFrame(const Work_ptr& work){

  if(_audioJB!=nullptr && _isRunning){
     _audioJB->add(work);
   }
}

void AgoraIo::receiveVideoFrame(const uint userId, const uint8_t* buffer,
                                        const size_t& length,const int& isKeyFrame,
                                        const uint64_t& ts){


      if(_receivedVideoFrames->size()>1 && _verbose){
        std::cout<<"video buffer size: "<<_receivedVideoFrames->size()<<std::endl;
      }

      _lastReceivedFrameTime=Now();

      //do not read video if the pipeline is in pause state
      if(_isPaused) return;

      const size_t MAX_BUFFER_SIZE=200;
      if(_receivedVideoFrames->size()<MAX_BUFFER_SIZE){

             auto frame=std::make_shared<Work>(buffer, length,isKeyFrame);
             frame->timestamp=ts;
             _receivedVideoFrames->add(frame);
       }
       else if(_verbose){
             std::cout<<"video buffer reached max size"<<std::endl;
      }
}

void AgoraIo::receiveAudioFrame(const uint userId, const uint8_t* buffer,
                                          const size_t& length){

         //do not read audio if the pipeline is in pause state
         if(_isPaused) return;

         const size_t MAX_BUFFER_SIZE=200;
         if(_receivedAudioFrames->size()<MAX_BUFFER_SIZE){

             auto frame=std::make_shared<Work>(buffer, length,false);
             _receivedAudioFrames->add(frame);
         }
         else if(_verbose){
             std::cout<<"audio buffer reached max size"<<std::endl;
         }
}

void AgoraIo::handleUserStateChange(const std::string& userId, 
                                              const UserState& newState){

    if(newState==USER_JOIN){
        subscribeAudioUser(userId);
    }  

    //we monitor user volumes only for those who have camera events
    if(newState==USER_CAM_ON){
        _pcmFrameObserver->onUserJoined(userId);
    }  
    else if(newState==USER_CAM_OFF){
      _pcmFrameObserver->onUserLeft(userId);
    }                                           

    if(newState==USER_JOIN || newState==USER_CAM_ON){

        //if there is not active user we are subscribing to, subscribe to this user
        if(_activeUsers.empty()){
            subscribeToVideoUser(userId);
        }

        _activeUsers.emplace_back(userId);  
    }
    else if(newState==USER_LEAVE || USER_CAM_OFF){

         _connection->getLocalUser()->unsubscribeVideo(userId.c_str());
        _activeUsers.remove_if([userId](const std::string& id){ return (userId==id); });
        if(_activeUsers.empty()==false && _currentVideoUser==userId){

            auto newUserId=_activeUsers.front();
            subscribeToVideoUser(newUserId);
        }
    } 
}

 void AgoraIo::subscribeToVideoUser(const std::string& userId){

    agora::rtc::ILocalUser::VideoSubscriptionOptions subscriptionOptions;
    subscriptionOptions.encodedFrameOnly = true;
    subscriptionOptions.type = agora::rtc::VIDEO_STREAM_HIGH;
    _connection->getLocalUser()->subscribeVideo(userId.c_str(), subscriptionOptions);

    _currentVideoUser=userId;
    std::cout<<"subscribed to video user #"<<_currentVideoUser<<std::endl;

    addEvent(AGORA_EVENT_ON_VIDEO_SUBSCRIBED,userId,0,0);
 }



void AgoraIo::setOnAudioFrameReceivedFn(const OnNewAudioFrame_fn& fn){
   _pcmFrameObserver->setOnAudioFrameReceivedFn(fn);
}

void AgoraIo::setOnVideoFrameReceivedFn(const OnNewFrame_fn& fn){
  h264FrameReceiver->setOnVideoFrameReceivedFn(fn);
}

size_t AgoraIo::getNextVideoFrame(unsigned char* data,
                                  size_t max_buffer_size,
                                  int* is_key_frame,
                                  uint64_t* ts){
   
    //do not wait for frames to arrive
    if(_receivedVideoFrames->isEmpty()){
        return 0;
    }

    _receivedVideoFrames->waitForWork();
    Work_ptr work=_receivedVideoFrames->get();

    memcpy(data, work->buffer, work->len);

    *is_key_frame=work->is_key_frame;
    *ts=work->timestamp;

    return work->len;
}

size_t AgoraIo::getNextAudioFrame(uint8_t* data, size_t max_buffer_size){
   
    _receivedAudioFrames->waitForWork();
    Work_ptr work=_receivedAudioFrames->get();

    memcpy(data, work->buffer, work->len);

    return work->len;
}

void AgoraIo::subscribeAudioUser(const std::string& userId){

    _connection->getLocalUser()->subscribeAudio(userId.c_str());
    std::cout<<"subscribed to audio user "<<userId<<std::endl;

    //_subscribedAudioUsers.emplace_back(userId);
}
void AgoraIo::unsubscribeAudioUser(const std::string& userId){

  _connection->getLocalUser()->unsubscribeAudio(userId.c_str());
}

bool AgoraIo::doSendHighVideo(const uint8_t* buffer,  uint64_t len,int is_key_frame){

  auto frameType=agora::rtc::VIDEO_FRAME_TYPE_DELTA_FRAME; 
  if(is_key_frame){
     frameType=agora::rtc::VIDEO_FRAME_TYPE_KEY_FRAME;
  }

  agora::rtc::EncodedVideoFrameInfo videoEncodedFrameInfo;
  videoEncodedFrameInfo.rotation = agora::rtc::VIDEO_ORIENTATION_0;
  videoEncodedFrameInfo.codecType = agora::rtc::VIDEO_CODEC_H264;
  videoEncodedFrameInfo.framesPerSecond = 30;
  videoEncodedFrameInfo.frameType = frameType;
  videoEncodedFrameInfo.streamType = agora::rtc::VIDEO_STREAM_HIGH;

  _videoFrameSender->sendEncodedVideoImage(buffer,len,videoEncodedFrameInfo);

  return true;
}

bool AgoraIo::doSendAudio(const uint8_t* buffer,  uint64_t len){

  agora::rtc::EncodedAudioFrameInfo audioFrameInfo;
  audioFrameInfo.numberOfChannels =1; //TODO
  audioFrameInfo.sampleRateHz = 48000; //TODO
  audioFrameInfo.codec = agora::rtc::AUDIO_CODEC_OPUS;

  _audioSender->sendEncodedAudioFrame(buffer,len, audioFrameInfo);

  return true;
}

int AgoraIo::sendVideo(const uint8_t * buffer,  
                              uint64_t len,
                              int is_key_frame,
                              long timestamp){

   if(_videoJB!=nullptr && _isRunning){
      
      Work_ptr work=std::make_shared<Work>(buffer,len, is_key_frame);
      work->timestamp=timestamp;
      _videoJB->add(work);

     }


   return 0; //no errors
}

/*
 * buffer size doubling logic;
     input: jb-initial-size-ms -- intial buffer size in ms
            jb-max-size-ms     -- max buffer size in ms
            Jb-max-doubles-if-emptied-within-seconds 
                 -- time limit where buffer size is doupled if it becomes empty within it
      logic:
            jb-size=jb-initial-size-ms
            while (running)
               if no frames arrived withing the last 3 seconds and the buffer is empty
                  set require-buffering=true
                if require-buffering 
                   wait for the buffer to be filled up for max 200 iterations 
                   if last-buffering-time < Jb-max-doubles-if-emptied-within-seconds 
                                               AND jb-size <jb-max-size-ms
                       jb-size =jb-size*2


 */

void AgoraIo::VideoThreadHandlerHigh(){

   _lastVideoSendTime=Now();
   uint8_t currentFramePerSecond=0;

   while(_isRunning==true){

     _lastVideoSendTime=Now();

     //wait untill work is available
     _videoJB->waitForWork();	  
     Work_ptr work=_videoJB->get();
     
     TimePoint  nextSample = Now()+std::chrono::milliseconds(30);

     if(work==nullptr) continue;
     if(work->is_finished==1) break;

     doSendHighVideo(work->buffer, work->len, (bool)(work->is_key_frame));

     //sleep until our next frame time
     std::this_thread::sleep_until(nextSample);
  }

  logMessage("VideoThreadHandlerHigh ended");
}

void AgoraIo::AudioThreadHandler(){

   const int waitTimeForBufferToBeFull=10;
   
   while(_isRunning==true){

     //wait untill work is available
     _audioJB->waitForWork();
     Work_ptr work=_audioJB->get();
     if(work==NULL) continue;

     if(work->is_finished){
        return;
     }

     doSendAudio(work->buffer, work->len);
   }
}
void AgoraIo::disconnect(){

    logMessage("start agora disonnect ...");

   _isRunning=false;
   //tell the thread that we are finished
    Work_ptr work=std::make_shared<Work>(nullptr,0, false);
    work->is_finished=true;

   _videoJB->add(work);
   _audioJB->add(work);

   std::this_thread::sleep_for(std::chrono::seconds(2));

   _connection->getLocalUser()->unpublishAudio(_customAudioTrack);
   _connection->getLocalUser()->unpublishVideo(_customVideoTrack);

   bool  isdisconnected=_connection->disconnect();
   if(isdisconnected){
      return;
   }

   _connectionObserver.reset();
   _userObserver.reset();

   _audioSender = nullptr;
   _videoFrameSender = nullptr;
   _customAudioTrack = nullptr;
   _customVideoTrack = nullptr;


   _videoJB=nullptr;;
   _audioJB=nullptr;;

   //delete context
   _connection=nullptr;

   _service->release();
   _service = nullptr;
  
   _videoThreadHigh->detach();
   _audioThread->detach();

   _videoThreadHigh=nullptr;;
   _audioThread=nullptr; 

   h264FrameReceiver=nullptr;

   std::cout<<"agora disconnected\n";

   logMessage("Agora disonnected ");
}

void agora_log_message(const char* message){

   /*if(ctx->callConfig->useDetailedAudioLog()){
      logMessage(std::string(message));
   }*/
}

void AgoraIo::unsubscribeAllVideo(){

    _connection->getLocalUser()->unsubscribeAllVideo(); 
}
void AgoraIo::setPaused(const bool& flag){

    _isPaused=flag;
    if(_isPaused==true){
        unsubscribeAllVideo();
    }
    else{

       unsubscribeAllVideo();
       if(_currentVideoUser!=""){
           subscribeToVideoUser(_currentVideoUser);
       } 
    }
}

void AgoraIo::addEvent(const AgoraEventType& eventType, 
                       const std::string& userName,
                       const long& param1, 
                       const long& param2,
                       long* states){

    if(_eventfn!=nullptr){
        _eventfn(_userEventData, eventType, userName.c_str(), param1, param2, states);
    }
}

 void AgoraIo::setEventFunction(event_fn fn, void* userData){

     _userEventData=userData;
     _eventfn=fn;
 }


