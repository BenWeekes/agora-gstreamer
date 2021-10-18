
#include "agorareceiver.h"
#include <iostream>
#include "helpers/agoralog.h"
#include "helpers/uidtofile.h"

//AgoraReceiverUser
AgoraReceiverUser::AgoraReceiverUser(const std::string& appId, 
                                     const std::string& channel,
                                     const std::string& userId,
                                     bool receiveAudio,
                                     bool receiveVideo,
                                     bool verbose,
                                     const std::string& filePath) :
_appId(appId),
_channel(channel),
_userId(userId),
_receiveAudio(receiveAudio),
_receiveVideo(receiveVideo),
_verbose(verbose),
_lastReceivedFrameTime(Now()),
_filePath(filePath),
_currentVideoUser(""),
_currentAgoraSink("")

{
     _activeUsers.clear();
}

AgoraReceiverUser::~AgoraReceiverUser()
{
}

bool AgoraReceiverUser::doConnect()
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
    scfg.appId = _appId.c_str();
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

bool AgoraReceiverUser::connect()
{
    if(!doConnect()){
        return false;
    }
    
    //_rtcConfig.clientRoleType = agora::rtc::CLIENT_ROLE_AUDIENCE;
   // _rtcConfig.channelProfile = agora::CHANNEL_PROFILE_COMMUNICATION;
    _rtcConfig.clientRoleType = agora::rtc::CLIENT_ROLE_BROADCASTER;

    _rtcConfig.autoSubscribeAudio = false;
    _rtcConfig.autoSubscribeVideo = false;
    _rtcConfig.enableAudioRecordingOrPlayout = false; 
    
    _currentAgoraSink=ReadCurrentUid();
    
    std::cout<<"agorasink user id: "<<_currentAgoraSink<<std::endl;

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

   if(_receiveVideo==true){
      //video subscription option
      subscribeToVideoUser(_userId);
   }

   //configure audio receive logic
   if(_userId!=""){
       _connection->getLocalUser()->subscribeAudio(_userId.c_str());
   }

   _userObserver=std::make_shared<UserObserver>(_connection->getLocalUser());
   
    // _connection->getLocalUser()->subscribeAllAudio();

    //register audio observer
    _pcmFrameObserver = std::make_shared<PcmFrameObserver>(); 
    if (_connection->getLocalUser()->setPlaybackAudioFrameParameters(1, 48000) != 0) {
       logMessage("Agora: Failed to set audio frame parameters!");
       return false;
    }

    // Register connection observer to monitor connection event
    _connectionObserver = std::make_shared<ConnectionObserver>();
    _connection->registerObserver(_connectionObserver.get());

    _connection->getLocalUser()->registerAudioFrameObserver(_pcmFrameObserver.get());
    auto res = _connection->connect(_appId.c_str(), _channel.c_str(), "");
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

    // Publish  video track
    _connection->getLocalUser()->publishVideo(_customVideoTrack);


    localUserObserver = std::make_shared<UserObserver>(_connection->getLocalUser());

    h264FrameReceiver = std::make_shared<H264FrameReceiver>();
    localUserObserver->setVideoEncodedImageReceiver(h264FrameReceiver.get());

    //video
     _receivedVideoFrames=std::make_shared<WorkQueue <Work_ptr> >();
    h264FrameReceiver->setOnVideoFrameReceivedFn([this](const uint userId, 
                                                    const uint8_t* buffer,
                                                    const size_t& length,
                                                    const int& isKeyFrame){

           receiveVideoFrame(userId, buffer, length, isKeyFrame);

    });

   //audio
     _receivedAudioFrames=std::make_shared<WorkQueue <Work_ptr> >();
    _pcmFrameObserver->setOnAudioFrameReceivedFn([this](const uint userId, 
                                                    const uint8_t* buffer,
                                                    const size_t& length){

          //this check will be every second
          auto sinkUid=_sinkUidMonitor.checkAndReadUid();
          if(sinkUid!=""){
             _connection->getLocalUser()->unsubscribeAudio(sinkUid.c_str());
             //std::cout<<"unsubscribed to audio user "<<sinkUid<<std::endl;
          }
          receiveAudioFrame(userId, buffer, length);
    });

    //connection observer: handles user join and leave
    _connectionObserver->setOnUserStateChanged([this](const std::string& userId,
                                                      const UserState& newState){
        handleUserStateChange(userId, newState);
    });

    _userObserver->setOnUserInfofn([this](const std::string& userId, const int& messsage, const int& value){
        if(messsage==1 && value==1){
           handleUserStateChange(userId, USER_LEAVE);
        }
    });

    _connected = true;

    return _connected;
}

void AgoraReceiverUser::receiveVideoFrame(const uint userId, const uint8_t* buffer,
                                          const size_t& length,const int& isKeyFrame){
      if(_receiveVideo==false){
            return ;
       }

      if(_verbose==true){
             auto  timeDiff=GetTimeDiff(_lastReceivedFrameTime,Now());
             std::cout<<"Time since last frame (ms): "<<timeDiff<<std::endl;
             logMessage("Time since last frame (ms): "+std::to_string(timeDiff));
       }

      _lastReceivedFrameTime=Now();

      const size_t MAX_BUFFER_SIZE=200;
      if(_receivedVideoFrames->size()<MAX_BUFFER_SIZE){

             auto frame=std::make_shared<Work>(buffer, length,isKeyFrame);
             _receivedVideoFrames->add(frame);
       }
       else{
             std::cout<<"video buffer reached max size"<<std::endl;
      }
}

void AgoraReceiverUser::receiveAudioFrame(const uint userId, const uint8_t* buffer,
                                          const size_t& length){

         if(_receiveAudio==false){
             return;
         }

         const size_t MAX_BUFFER_SIZE=200;
         if(_receivedAudioFrames->size()<MAX_BUFFER_SIZE){

             auto frame=std::make_shared<Work>(buffer, length,false);
             _receivedAudioFrames->add(frame);
         }
         else{
             std::cout<<"audio buffer reached max size"<<std::endl;
         }

}

void AgoraReceiverUser::handleUserStateChange(const std::string& userId, 
                                              const UserState& newState){

    if(newState==USER_JOIN && _receiveAudio==true){
        _currentAgoraSink=ReadCurrentUid();
        if(userId!=_currentAgoraSink){
            _connection->getLocalUser()->subscribeAudio(userId.c_str());
            std::cout<<"subscribed to audio user "<<userId<<std::endl;
        } 
    }                                              
    //is a user id is provided by the plugin, we do not need to subscribe to someone else
    if(_userId!="" || _receiveVideo==false)  return;

    if(newState==USER_JOIN){

        //if there is not active user we are subscribing to, subscribe to this user
        if(_activeUsers.empty()){
            subscribeToVideoUser(userId);
        }

        _activeUsers.emplace_back(userId);  
    }
    else if(newState==USER_LEAVE){

        _activeUsers.remove_if([userId](const std::string& id){ return (userId==id); });
        if(_activeUsers.empty()==false && _currentVideoUser==userId){

            auto newUserId=_activeUsers.front();
            subscribeToVideoUser(newUserId);
        }
    } 
}

 void AgoraReceiverUser::subscribeToVideoUser(const std::string& userId){

    agora::rtc::ILocalUser::VideoSubscriptionOptions subscriptionOptions;
    subscriptionOptions.encodedFrameOnly = true;
    subscriptionOptions.type = agora::rtc::VIDEO_STREAM_HIGH;
    _connection->getLocalUser()->subscribeVideo(userId.c_str(), subscriptionOptions);

    _currentVideoUser=userId;
    std::cout<<"subscribed to video user #"<<_currentVideoUser<<std::endl;
 }

bool AgoraReceiverUser::disconnect(){

    // Unregister video frame observers
    localUserObserver->unsetVideoFrameObserver();
    
    // Disconnect from Agora channel
    auto res = _connection->disconnect();;
    if (res < 0)
    {
        logMessage("Error disconnecting from receiver channel");
        return false;
    }

    logMessage("Disconnected from receiver channel");

    // Destroy Agora connection and related resources
    localUserObserver.reset();
    h264FrameReceiver.reset();
    _pcmFrameObserver.reset();

    _connection = nullptr;

    // Destroy Agora Service
    _service->release();
    _service = nullptr;

    return true;
}

void AgoraReceiverUser::setOnAudioFrameReceivedFn(const OnNewAudioFrame_fn& fn){
   _pcmFrameObserver->setOnAudioFrameReceivedFn(fn);
}

void AgoraReceiverUser::setOnVideoFrameReceivedFn(const OnNewFrame_fn& fn){
  h264FrameReceiver->setOnVideoFrameReceivedFn(fn);
}

size_t AgoraReceiverUser::getNextVideoFrame(unsigned char* data, size_t max_buffer_size, int* is_key_frame){
   
    _receivedVideoFrames->waitForWork();
    Work_ptr work=_receivedVideoFrames->get();

    memcpy(data, work->buffer, work->len);

    *is_key_frame=work->is_key_frame;

    return work->len;
}

size_t AgoraReceiverUser::getNextAudioFrame(unsigned char* data, size_t max_buffer_size){
   
    _receivedAudioFrames->waitForWork();
    Work_ptr work=_receivedAudioFrames->get();

    memcpy(data, work->buffer, work->len);

    return work->len;
}
