
#include "agorareceiver.h"
#include <iostream>
#include <chrono>
#include <functional>
#include <fstream>

//agora header files
#include "NGIAgoraRtcConnection.h"
#include "IAgoraService.h"
#include "AgoraBase.h"

#include "helpers/agoralog.h"

#include "userobserver.h"
#include "helpers/context.h"

using OnNewFrame_fn=std::function<void(const uint userId, 
                                        const uint8_t* buffer,
                                        const size_t& size,
                                        const int isKeyFrame)>;

class H264FrameReceiver : public agora::rtc::IVideoEncodedImageReceiver
{
public:
    H264FrameReceiver();

    bool OnEncodedVideoImageReceived(const uint8_t* imageBuffer, size_t length, 
        const agora::rtc::EncodedVideoFrameInfo& videoEncodedFrameInfo) override;

    void setOnVideoFrameReceivedFn(const OnNewFrame_fn& fn);

private:
    OnNewFrame_fn                   _onVideoFrameReceived;
};

//audio receiver
class PcmFrameObserver : public agora::media::IAudioFrameObserver {
 public:
  PcmFrameObserver(): _onAudioFrameReceived(nullptr){}

  bool onPlaybackAudioFrame(AudioFrame& audioFrame) override { return true; };

  bool onRecordAudioFrame(AudioFrame& audioFrame) override { return true; };

  bool onMixedAudioFrame(AudioFrame& audioFrame) override { return true; };

  bool onPlaybackAudioFrameBeforeMixing(unsigned int uid, AudioFrame& audioFrame) override;

  int calcVol(const int16_t* newPacket, const uint16_t& packetLen);

  void setOnAudioFrameReceivedFn(const OnNewFrame_fn& fn);

private:

  OnNewFrame_fn                   _onAudioFrameReceived;
};

using PcmFrameObserver_ptr=std::shared_ptr<PcmFrameObserver>;

class AgoraReceiverUser 
{
public:
    AgoraReceiverUser(const std::string& _appId,
                      const std::string& _channel,
                      const std::string& _userId);

    virtual ~AgoraReceiverUser();

    virtual bool connect();
    virtual bool disconnect();

    void setOnAudioFrameReceivedFn(const OnNewFrame_fn& fn);
    void setOnVideoFrameReceivedFn(const OnNewFrame_fn& fn);

    size_t getNextVideoFrame(unsigned char* data, size_t max_buffer_size, int* is_key_frame);

protected:

    bool doConnect();

private:
    std::shared_ptr<H264FrameReceiver>  h264FrameReceiver;
    std::shared_ptr<UserObserver>       localUserObserver;

    PcmFrameObserver_ptr                 _pcmFrameObserver;

    std::string                          _appId;
    std::string                          _channel;
    std::string                          _userId;

    agora::base::IAgoraService*                     _service;
    agora::agora_refptr<agora::rtc::IRtcConnection> _connection;
    agora::rtc::RtcConnectionConfiguration          _rtcConfig;

    agora::agora_refptr<agora::rtc::IMediaNodeFactory> _factory;
    agora::agora_refptr<agora::rtc::ILocalAudioTrack> _customAudioTrack;
    agora::agora_refptr<agora::rtc::ILocalVideoTrack> _customVideoTrack;

    bool                                           _connected = false;

     WorkQueue_ptr                                 _receivedVideoFrames;
};

//H264FrameReceiver

H264FrameReceiver::H264FrameReceiver()
{
}

bool H264FrameReceiver::OnEncodedVideoImageReceived(const uint8_t* imageBuffer, size_t length, 
    const agora::rtc::EncodedVideoFrameInfo& videoEncodedFrameInfo)
{
    if (!_onVideoFrameReceived)
        return false;

    bool isKeyFrame=videoEncodedFrameInfo.frameType == agora::rtc::VIDEO_FRAME_TYPE_KEY_FRAME;
    
    for(int i=0;i<10;i++)
      std::cout<<(int)(imageBuffer[i])<<" ";
    std::cout<<std::endl;

    _onVideoFrameReceived(videoEncodedFrameInfo.uid, imageBuffer, length,isKeyFrame);

    std::cout<<"Res:"<<videoEncodedFrameInfo.width<<"x"<<videoEncodedFrameInfo.height<<std::endl;

    return true;
}

void H264FrameReceiver::setOnVideoFrameReceivedFn(const OnNewFrame_fn& fn){
   _onVideoFrameReceived=fn;
}

void PcmFrameObserver::setOnAudioFrameReceivedFn(const OnNewFrame_fn& fn){
   _onAudioFrameReceived=fn;
}

bool PcmFrameObserver::onPlaybackAudioFrameBeforeMixing(unsigned int uid, AudioFrame& audioFrame) {
   
   if(_onAudioFrameReceived!=nullptr){


      //TODO: is user id is all the time int?
      //_onAudioFrameReceived(uid, frame);
   }
  
  return true;
}

//AgoraReceiverUser
AgoraReceiverUser::AgoraReceiverUser(const std::string& appId, const std::string& channel, const std::string& userId) :
_appId(appId),
_channel(channel),
_userId(userId)
{
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
    //TODO: make a connection here 
    if(!doConnect()){
        return false;
    }
    
    _rtcConfig.clientRoleType = agora::rtc::CLIENT_ROLE_AUDIENCE;
    _rtcConfig.autoSubscribeAudio = true;
    _rtcConfig.autoSubscribeVideo = false;
    _rtcConfig.enableAudioRecordingOrPlayout = false;  // Subscribe audio but without playback

    _connection = _service->createRtcConnection(_rtcConfig);
    if (!_connection)
    {
        logMessage("Error creating connection to Agora SDK");
        return false;
    }
    
    auto factory = _service->createMediaNodeFactory();
    if (!factory)
    {
        logMessage("Error creating factory");
        return false;
    }

    agora::rtc::ILocalUser::VideoSubscriptionOptions subscriptionOptions;
    subscriptionOptions.encodedFrameOnly = true;
    subscriptionOptions.type = agora::rtc::VIDEO_STREAM_HIGH;

   // _connection->getLocalUser()->subscribeAllVideo(subscriptionOptions);
   _connection->getLocalUser()->subscribeVideo(_userId.c_str(), subscriptionOptions);
   std::cout<<"Receive video from user #"<<_userId<<std::endl;

    //register audio observer
    _pcmFrameObserver = std::make_shared<PcmFrameObserver>(); 
    if (_connection->getLocalUser()->setPlaybackAudioFrameBeforeMixingParameters(1, 48000) != 0) {
       logMessage("Agora: Failed to set audio frame parameters!");
       return false;
    }

    _connection->getLocalUser()->registerAudioFrameObserver(_pcmFrameObserver.get());
    auto res = _connection->connect(_appId.c_str(), _channel.c_str(), "");
    if (res)
    {
       logMessage("Error connecting to channel");
        return false;
    }

    localUserObserver = std::make_shared<UserObserver>(_connection->getLocalUser());

    h264FrameReceiver = std::make_shared<H264FrameReceiver>();
    localUserObserver->setVideoEncodedImageReceiver(h264FrameReceiver.get());

     _receivedVideoFrames=std::make_shared<WorkQueue <Work_ptr> >();
    h264FrameReceiver->setOnVideoFrameReceivedFn([this](const uint userId, 
                                                    const uint8_t* buffer,
                                                    const size_t& length,
                                                    const int& isKeyFrame){

         const size_t MAX_BUFFER_SIZE=200;
         if(_receivedVideoFrames->size()<MAX_BUFFER_SIZE){

             auto frame=std::make_shared<Work>(buffer, length,isKeyFrame);
             _receivedVideoFrames->add(frame);
         }
         else{
             std::cout<<"buffer reached max size"<<std::endl;
         }

    });

    _connected = true;

    return _connected;
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

void AgoraReceiverUser::setOnAudioFrameReceivedFn(const OnNewFrame_fn& fn){
   _pcmFrameObserver->setOnAudioFrameReceivedFn(fn);
}

void AgoraReceiverUser::setOnVideoFrameReceivedFn(const OnNewFrame_fn& fn){
  h264FrameReceiver->setOnVideoFrameReceivedFn(fn);
}

std::shared_ptr<AgoraReceiverUser> create_receive_user(const std::string& _appId,
                                                       const std::string& _channel,
                                                       const std::string& _userId){
  
    std::shared_ptr<AgoraReceiverUser> receiver=std::make_shared<AgoraReceiverUser>(_appId, _channel, _userId); 

    if(!receiver->connect()){
       return nullptr;
    }

    return receiver;                                                                                                
}

size_t AgoraReceiverUser::getNextVideoFrame(unsigned char* data, size_t max_buffer_size, int* is_key_frame){
   
    _receivedVideoFrames->waitForWork();
    Work_ptr work=_receivedVideoFrames->get();

    memcpy(data, work->buffer, work->len);

    *is_key_frame=work->is_key_frame;

    return work->len;
}

size_t get_next_video_frame(std::shared_ptr<AgoraReceiverUser> receiver, 
              unsigned char* data, size_t max_buffer_size, int* is_key_frame){

   return receiver->getNextVideoFrame(data, max_buffer_size, is_key_frame);

}
