
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
#include "helpers/utilities.h"

#include "file_parser/helper_h264_parser.h"

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
                      const std::string& _userId,
                      bool receiveAudio=false,
                      bool receiveVideo=false,
                      bool verbose=false,
                      const std::string& filePath="");

    virtual ~AgoraReceiverUser();

    virtual bool connect();
    virtual bool disconnect();

    void setOnAudioFrameReceivedFn(const OnNewFrame_fn& fn);
    void setOnVideoFrameReceivedFn(const OnNewFrame_fn& fn);

    size_t getNextVideoFrame(unsigned char* data, size_t max_buffer_size, int* is_key_frame);

    size_t getNextAudioFrame(unsigned char* data, size_t max_buffer_size);

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
     WorkQueue_ptr                                 _receivedAudioFrames;

     bool                                          _receiveAudio;
     bool                                          _receiveVideo;
     bool                                          _verbose;

     TimePoint                                     _lastReceivedFrameTime;

     agora::agora_refptr<agora::rtc::IVideoEncodedImageSender> _videoFrameSender;

     std::shared_ptr<std::thread>      _senderThread;
     std::string                       _filePath;
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
    
    _onVideoFrameReceived(videoEncodedFrameInfo.uid, imageBuffer, length,isKeyFrame);

    return true;
}

bool PcmFrameObserver::onPlaybackAudioFrameBeforeMixing(unsigned int uid, AudioFrame& audioFrame) {
   
   if(_onAudioFrameReceived!=nullptr){

        _onAudioFrameReceived(uid, (const unsigned char*)audioFrame.buffer, audioFrame.samplesPerChannel*2,0);        
   }
  
  return true;
}

void H264FrameReceiver::setOnVideoFrameReceivedFn(const OnNewFrame_fn& fn){
   _onVideoFrameReceived=fn;
}

void PcmFrameObserver::setOnAudioFrameReceivedFn(const OnNewFrame_fn& fn){
   _onAudioFrameReceived=fn;
}

///===============testing ===================
struct PacerInfo {
  int sendTimes;
  int sendIntervalInMs;
  std::chrono::steady_clock::time_point startTime;
};

void waitBeforeNextSend(PacerInfo& pacer) {
  auto sendFrameEndTime = std::chrono::steady_clock::now();
  int nextDurationInMs = (++pacer.sendTimes * pacer.sendIntervalInMs);
  int waitIntervalInMs = nextDurationInMs - std::chrono::duration_cast<std::chrono::milliseconds>(
                                                sendFrameEndTime - pacer.startTime)
                                                .count();
  if (waitIntervalInMs > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(waitIntervalInMs));
  }
}

static void sendOneH264Frame(
    int frameRate, std::unique_ptr<HelperH264Frame> h264Frame,
    agora::agora_refptr<agora::rtc::IVideoEncodedImageSender>
        videoH264FrameSender,
    agora::rtc::VIDEO_STREAM_TYPE streamtype = agora::rtc::VIDEO_STREAM_HIGH) {
  agora::rtc::EncodedVideoFrameInfo videoEncodedFrameInfo;
  videoEncodedFrameInfo.rotation = agora::rtc::VIDEO_ORIENTATION_0;
  videoEncodedFrameInfo.codecType = agora::rtc::VIDEO_CODEC_H264;
  videoEncodedFrameInfo.framesPerSecond = frameRate;
  videoEncodedFrameInfo.streamType = streamtype;
  videoEncodedFrameInfo.frameType =
      (h264Frame.get()->isKeyFrame
           ? agora::rtc::VIDEO_FRAME_TYPE::VIDEO_FRAME_TYPE_KEY_FRAME
           : agora::rtc::VIDEO_FRAME_TYPE::VIDEO_FRAME_TYPE_DELTA_FRAME);


  videoH264FrameSender->sendEncodedVideoImage(
      reinterpret_cast<uint8_t*>(h264Frame.get()->buffer.get()),
      h264Frame.get()->bufferLen, videoEncodedFrameInfo);
}

static void SampleSendVideoH264Task(const std::string & fileName,
    agora::agora_refptr<agora::rtc::IVideoEncodedImageSender>
        videoH264FrameSender,
    bool& exitFlag, agora::rtc::VIDEO_STREAM_TYPE streamtype)
{
  
  std::unique_ptr<HelperH264FileParser> h264FileParser(
      new HelperH264FileParser(fileName.c_str()));
  h264FileParser->initialize();

  // Calculate send interval based on frame rate. H264 frames are sent at this
  // interval
  PacerInfo pacer = {0, 1000 / 30,
                     std::chrono::steady_clock::now()};

  while (!exitFlag) {
    if (auto h264Frame = h264FileParser->getH264Frame()) {
      sendOneH264Frame(30, std::move(h264Frame),
                       videoH264FrameSender, streamtype);
      waitBeforeNextSend(pacer);  // sleep for a while before sending next frame
    }
  };
}
//========================end of testing ===========

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
_filePath(filePath)

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

static bool exitFlag = false;

bool AgoraReceiverUser::connect()
{
    //TODO: make a connection here 
    if(!doConnect()){
        return false;
    }
    
    //_rtcConfig.clientRoleType = agora::rtc::CLIENT_ROLE_AUDIENCE;
   // _rtcConfig.channelProfile = agora::CHANNEL_PROFILE_COMMUNICATION;
    _rtcConfig.clientRoleType = agora::rtc::CLIENT_ROLE_BROADCASTER;

    _rtcConfig.autoSubscribeAudio = true;
    _rtcConfig.autoSubscribeVideo = false;
    _rtcConfig.enableAudioRecordingOrPlayout = false;  // Subscribe audio but without playback

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

  
    //TODO: test sending dummy
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


    _senderThread=std::make_shared<std::thread>(SampleSendVideoH264Task,
                              _filePath, 
                              _videoFrameSender, std::ref(exitFlag),
                              agora::rtc::VIDEO_STREAM_HIGH);


    localUserObserver = std::make_shared<UserObserver>(_connection->getLocalUser());

    h264FrameReceiver = std::make_shared<H264FrameReceiver>();
    localUserObserver->setVideoEncodedImageReceiver(h264FrameReceiver.get());

    //video
     _receivedVideoFrames=std::make_shared<WorkQueue <Work_ptr> >();
    h264FrameReceiver->setOnVideoFrameReceivedFn([this](const uint userId, 
                                                    const uint8_t* buffer,
                                                    const size_t& length,
                                                    const int& isKeyFrame){

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
             std::cout<<"buffer reached max size"<<std::endl;
         }

    });

   //audio
     _receivedAudioFrames=std::make_shared<WorkQueue <Work_ptr> >();
    _pcmFrameObserver->setOnAudioFrameReceivedFn([this](const uint userId, 
                                                    const uint8_t* buffer,
                                                    const size_t& length,
                                                    const int& isKeyFrame){


         //we read audio only from this user id
         if(_receiveAudio==false || _userId!=std::to_string(userId)){
             return;
         }

         const size_t MAX_BUFFER_SIZE=200;
         if(_receivedAudioFrames->size()<MAX_BUFFER_SIZE){

             auto frame=std::make_shared<Work>(buffer, length,isKeyFrame);
             _receivedAudioFrames->add(frame);
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
                                                       const std::string& _userId,
                                                       int receiveAudio,
													   int receiveVideo,
                                                       int verbose,
                                                       const std::string& filePath){
  
    std::shared_ptr<AgoraReceiverUser> receiver=
      std::make_shared<AgoraReceiverUser>(_appId, _channel, _userId,
                                          receiveAudio, receiveVideo,
                                          verbose,
                                          filePath); 

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

size_t AgoraReceiverUser::getNextAudioFrame(unsigned char* data, size_t max_buffer_size){
   
    _receivedAudioFrames->waitForWork();
    Work_ptr work=_receivedAudioFrames->get();

    memcpy(data, work->buffer, work->len);

    return work->len;
}

size_t get_next_video_frame(std::shared_ptr<AgoraReceiverUser> receiver, 
              unsigned char* data, size_t max_buffer_size, int* is_key_frame){

   return receiver->getNextVideoFrame(data, max_buffer_size, is_key_frame);

}

size_t get_next_audio_frame(std::shared_ptr<AgoraReceiverUser> receiver, 
                             unsigned char* data, size_t max_buffer_size){

    return receiver->getNextAudioFrame(data, max_buffer_size);
}
