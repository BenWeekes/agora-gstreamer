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



AgoraIo::AgoraIo(const bool& verbose):
 _verbose(verbose),
 _lastReceivedFrameTime(Now()),
 _currentVideoUser(""),
 _lastVideoUserSwitchTime(Now()){

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
agora_context_t*  AgoraIo::init(char* in_app_id, 
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

    _ctx=new agora_context_t;

    if(!doConnect(in_app_id)){
        return NULL;
    }

    std::string _userId=in_user_id;
    
    _rtcConfig.clientRoleType = agora::rtc::CLIENT_ROLE_BROADCASTER;

    _rtcConfig.autoSubscribeAudio = false;
    _rtcConfig.autoSubscribeVideo = false;
    _rtcConfig.enableAudioRecordingOrPlayout = false; 

    _ctx->enable_dual=enable_dual;

    //create a local config
    _ctx->callConfig=std::make_shared<LocalConfig>();
    _ctx->callConfig->loadConfig("/usr/local/nginx/conf/rtmpg.conf");
    _ctx->callConfig->print();

    _ctx->jb_size=_ctx->callConfig->getInitialJbSize();
    
    
    _connection = _service->createRtcConnection(_rtcConfig);
    if (!_connection)
    {
        logMessage("Error creating connection to Agora SDK");
        return NULL;
    }
    
    _factory = _service->createMediaNodeFactory();
    if (!_factory)
    {
        logMessage("Error creating factory");
        return NULL;
    }

   _userObserver=std::make_shared<UserObserver>(_connection->getLocalUser(),_verbose);
   
    //register audio observer
    _pcmFrameObserver = std::make_shared<PcmFrameObserver>();
    if (_connection->getLocalUser()->setPlaybackAudioFrameParameters(1, 48000) != 0) {
        logMessage("Agora: Failed to set audio frame parameters!");
        return NULL;
    }

    // Register connection observer to monitor connection event
    _connectionObserver = std::make_shared<ConnectionObserver>();
    _connection->registerObserver(_connectionObserver.get());

    _connection->getLocalUser()->registerAudioFrameObserver(_pcmFrameObserver.get());
    auto res = _connection->connect(in_app_id, in_ch_id, in_user_id);
    if (res)
    {
       logMessage("Error connecting to channel");
        return NULL;
    }

    _videoFrameSender=_factory->createVideoEncodedImageSender();
    if (!_videoFrameSender) {
       std::cout<<"Failed to create video frame sender!"<<std::endl;
       return NULL;
    }

    //if you want to send_dual_h264,the ccMode must be enabled
     agora::base::SenderOptions option;
     option.ccMode = agora::base::CC_ENABLED;
    // Create video track
    _customVideoTrack=_service->createCustomVideoTrack(_videoFrameSender, option);
    if (!_customVideoTrack) {
         std::cout<<"Failed to create video track!"<<std::endl;
         return NULL;
     }

     //audio
    // Create audio data sender
     _audioSender = _factory->createAudioEncodedFrameSender();
     if (!_audioSender) {
        return NULL;
      }

     // Create audio track
     _customAudioTrack =_service->createCustomAudioTrack(_audioSender, agora::base::MIX_DISABLED);
     if (!_customAudioTrack) {
        return NULL;
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
                                                    const int& isKeyFrame){

           receiveVideoFrame(userId, buffer, length, isKeyFrame);

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
        }
        else if(messsage==1 && value==0){
            handleUserStateChange(userId, USER_CAM_ON);
        }
    });

    _userObserver->setOnUserVolumeChangedFn([this](const std::string& userId, const int& volume){

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

  _ctx->isConnected=1;
  _ctx->videoJB=std::make_shared<WorkQueue <Work_ptr> >();
  _audioJB=std::make_shared<WorkQueue <Work_ptr> >();

  _ctx->isRunning=true;

    //start thread handlers
  _videoThreadHigh=std::make_shared<std::thread>(&AgoraIo::VideoThreadHandlerHigh, this,_ctx);
  _audioThread=std::make_shared<std::thread>(&AgoraIo::AudioThreadHandler,this, _ctx);

  _connected = true;

  _ctx->encodeNextFrame=true;

  _ctx->fps=0;
  _ctx->lastFpsPrintTime=Now();
  _ctx->lastBufferingTime=Now();
  _ctx->reBufferingCount=0;

  _ctx->lastFrameTimestamp=0; 
  _ctx->timestampPerSecond=0;

  //initially set to 30fps
  _ctx->predictedFps=30;

  _ctx->lastHighFrameSendTime=Now();
  _ctx->lastLowFrameSendTime=Now();

  _ctx->isJbBuffering=false;

  _ctx->lastVideoTimestamp=0;
  _ctx->lastAudioTimestamp=0;

  _ctx->highVideoFrameCount=0;
  _ctx->lowVideoFrameCount=0;

  _ctx->lastVideoSampingInterval=1000/(float)_ctx->predictedFps;

  _ctx->dfps=dfps;

  return _ctx;
}

void AgoraIo::addAudioFrame(const Work_ptr& work){

  _audioJB->add(work);
}

void AgoraIo::receiveVideoFrame(const uint userId, const uint8_t* buffer,
                                          const size_t& length,const int& isKeyFrame){
      if(_verbose==true){
             auto  timeDiff=GetTimeDiff(_lastReceivedFrameTime,Now());
             //std::cout<<"Time since last frame (ms): "<<timeDiff<<std::endl;
             //logMessage("Time since last frame (ms): "+std::to_string(timeDiff));
       }

      if(_receivedVideoFrames->size()>0 && _verbose){
        std::cout<<"video buffer size: "<<_receivedVideoFrames->size()<<std::endl;
      }

      _lastReceivedFrameTime=Now();

      const size_t MAX_BUFFER_SIZE=200;
      if(_receivedVideoFrames->size()<MAX_BUFFER_SIZE){

             auto frame=std::make_shared<Work>(buffer, length,isKeyFrame);
             _receivedVideoFrames->add(frame);
       }
       else if(_verbose){
             std::cout<<"video buffer reached max size"<<std::endl;
      }
}

void AgoraIo::receiveAudioFrame(const uint userId, const uint8_t* buffer,
                                          const size_t& length){

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
 }



void AgoraIo::setOnAudioFrameReceivedFn(const OnNewAudioFrame_fn& fn){
   _pcmFrameObserver->setOnAudioFrameReceivedFn(fn);
}

void AgoraIo::setOnVideoFrameReceivedFn(const OnNewFrame_fn& fn){
  h264FrameReceiver->setOnVideoFrameReceivedFn(fn);
}

size_t AgoraIo::getNextVideoFrame(unsigned char* data, size_t max_buffer_size, int* is_key_frame){
   
    //do not wait for frames to arrive
    if(_receivedVideoFrames->isEmpty()){
        return 0;
    }

    _receivedVideoFrames->waitForWork();
    Work_ptr work=_receivedVideoFrames->get();

    memcpy(data, work->buffer, work->len);

    *is_key_frame=work->is_key_frame;

    return work->len;
}

size_t AgoraIo::getNextAudioFrame(unsigned char* data, size_t max_buffer_size){
   
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


bool AgoraIo::doSendLowVideo(agora_context_t* ctx, const unsigned char* buffer,  unsigned long len,int is_key_frame){

  auto frameType=agora::rtc::VIDEO_FRAME_TYPE_DELTA_FRAME;
  if(is_key_frame){
     frameType=agora::rtc::VIDEO_FRAME_TYPE_KEY_FRAME;
  }

  ctx->videoDecoder->decode(buffer, len);
  auto frame=ctx->videoDecoder->getFrame();

  //check if we need to skip some frames
  ctx->encodeNextFrame=false;
  float lowHeighRatio= ctx->predictedFps/(float)ctx->dfps;
  if((ctx->lowVideoFrameCount*lowHeighRatio)<ctx->highVideoFrameCount){

      ctx->encodeNextFrame=true;
   }
  if( ctx->encodeNextFrame==true || is_key_frame){

    const uint32_t MAX_FRAME_SIZE=1024*1020;
    
    std::unique_ptr<uint8_t[]> outBuffer(new uint8_t[MAX_FRAME_SIZE]);

    //reencode
    uint32_t outBufferSize=0;
    ctx->videoEncoder->encode(frame, outBuffer.get(), outBufferSize,is_key_frame);
  
    agora::rtc::EncodedVideoFrameInfo videoEncodedFrameInfo;
    videoEncodedFrameInfo.rotation = agora::rtc::VIDEO_ORIENTATION_0;
    videoEncodedFrameInfo.codecType = agora::rtc::VIDEO_CODEC_H264;
    videoEncodedFrameInfo.framesPerSecond = ctx->dfps;
    videoEncodedFrameInfo.frameType = frameType;

    videoEncodedFrameInfo.streamType = agora::rtc::VIDEO_STREAM_LOW;
    _videoFrameSender->sendEncodedVideoImage(outBuffer.get(),outBufferSize,videoEncodedFrameInfo);

    ctx->lowVideoFrameCount++;
  }

  return true;

}

bool AgoraIo::doSendHighVideo(agora_context_t* ctx, const unsigned char* buffer,  unsigned long len,int is_key_frame){

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

bool AgoraIo::doSendAudio(agora_context_t* ctx, const unsigned char* buffer,  unsigned long len){

  agora::rtc::EncodedAudioFrameInfo audioFrameInfo;
  audioFrameInfo.numberOfChannels =1; //TODO
  audioFrameInfo.sampleRateHz = 48000; //TODO
  audioFrameInfo.codec = agora::rtc::AUDIO_CODEC_OPUS;

  _audioSender->sendEncodedAudioFrame(buffer,len, audioFrameInfo);

  return true;
}

void AgoraIo::UpdatePredictedFps(agora_context_t* ctx, const long& timestamp){

   const float maxFps=255.0;
   ctx->fps =ctx->fps+1;

   if(ctx->lastFrameTimestamp!=0){

     auto diff=timestamp-ctx->lastFrameTimestamp;
     ctx->timestampPerSecond +=diff;
   }

   ctx->lastFrameTimestamp=timestamp;
   if( GetTimeDiff(ctx->lastFpsPrintTime, Now())>=1000){

      float averageTimeStamp=ctx->timestampPerSecond/((float)ctx->fps);

       //safty check when  averageTimeStamp is not correct
       if(averageTimeStamp>1000/maxFps){ 
           ctx->predictedFps=1000/averageTimeStamp;
       }

      ctx->timestampPerSecond=0;
   }
}

int AgoraIo::agora_send_video(const unsigned char * buffer,  
                              unsigned long len,
                              int is_key_frame,
                              long timestamp){

   auto     timePerFrame=(1000/(float)_ctx->predictedFps);
   Work_ptr work=std::make_shared<Work>(buffer,len, is_key_frame);
   work->timestamp=timestamp;
   
   _ctx->videoJB->add(work);
 
   //update fps based on frame timestamp
   UpdatePredictedFps(_ctx, timestamp);
  
   //log current fps
   if(GetTimeDiff(_ctx->lastFpsPrintTime, Now())>=1000){

      if(_ctx->callConfig->useFpsLog() ){
         logMessage(GetAddressAsString(_ctx)+"JB-Video: (ms): " +
                           std::to_string((int)(_ctx->videoJB->size()*timePerFrame)) +
                            "/"+
                           std::to_string(_ctx->jb_size) +
                           ", fps="+std::to_string(_ctx->fps)+
                           ", predicted fps="+ std::to_string(_ctx->predictedFps));
     }

     _ctx->highVideoFrameCount=0;
     _ctx->lowVideoFrameCount=0;

     _ctx->lastFpsPrintTime=Now();
     _ctx->fps=0;

   }

   return 0; //no errors
}

int AgoraIo::agora_send_audio(agora_context_t* ctx,
                     const unsigned char * buffer, 
                     unsigned long len,
                     long timestamp){

    Work_ptr work=std::make_shared<Work>(buffer,len, 0);
    work->timestamp=timestamp;

    ctx->audioJB->add(work);

    return 0;
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
void AgoraIo::CheckAndFillInVideoJb(agora_context_t* ctx, const TimePoint& lastSendTime){

 bool  waitForBufferToBeFull=false;

  //check if no frames arrive for 3 seconds. If so, fill the buffer with frames
  auto diff=GetTimeDiff(lastSendTime, Now());
  if(diff>3*ctx->predictedFps && 
     ctx->videoJB->isEmpty()){

	    waitForBufferToBeFull=true;
  }

  //should wait for the buffer to have min size
  if(waitForBufferToBeFull==true ){
      WaitForBuffering(ctx);

      if(ctx->jb_size<ctx->callConfig->getMaxJbSize()){
         ResizeBuffer(ctx);
      }
  }
}

void AgoraIo::VideoThreadHandlerHigh(agora_context_t* ctx){

   TimePoint  lastSendTime=Now();
   uint8_t currentFramePerSecond=0;

   while(ctx->isRunning==true){

     auto   timePerFrame=(1000/(float)ctx->predictedFps);

     //check if we need to fill JB
     CheckAndFillInVideoJb(ctx, lastSendTime);
     if(ctx->callConfig->useDetailedVideoLog()){

        logMessage(GetAddressAsString(ctx)+"AGORA-JITTER: buffer size: " +
                           std::to_string(ctx->videoJB->size()*timePerFrame));
     }

     lastSendTime=Now();

     //wait untill work is available
     ctx->videoJB->waitForWork();	  
     Work_ptr work=ctx->videoJB->get();
     if(ctx->enable_dual){
         ctx->videoQueueLow->add(work);
      }

     
     TimePoint  nextSample = Now()+std::chrono::milliseconds(30);

     if(work==nullptr) continue;
     if(work->is_finished==1) break;

     doSendHighVideo(ctx,work->buffer, work->len, (bool)(work->is_key_frame));

     logMessage("send a video frame");

     auto delta=GetTimeDiff(ctx->lastHighFrameSendTime, Now());

     ctx->lastHighFrameSendTime=Now();
     ctx->lastVideoTimestamp=work->timestamp;

     ctx->highVideoFrameCount++;

      //logMessage("High video timestamp: "+std::to_string(work->timestamp)+", delta: "+std::to_string(delta));

     //sleep until our next frame time
     std::this_thread::sleep_until(nextSample);
  }

  logMessage("VideoThreadHandlerHigh ended");
}

void AgoraIo::VideoThreadHandlerLow(agora_context_t* ctx){

   while(ctx->isRunning==true){
 
     //wait untill work is available
     ctx->videoQueueLow->waitForWork();
     Work_ptr work=ctx->videoQueueLow->get();

     if(work==NULL) continue;

     if(work->is_finished==1){
        break;
     }

    doSendLowVideo(ctx,work->buffer, work->len, (bool)(work->is_key_frame));
    auto delta=GetTimeDiff(ctx->lastLowFrameSendTime, Now());
    ctx->lastLowFrameSendTime=Now();

  }

  logMessage("VideoThreadHandlerLow ended");
}

void AgoraIo::AudioThreadHandler(agora_context_t* ctx){

   const int waitTimeForBufferToBeFull=10;
   
   while(_ctx->isRunning==true){

     //wait untill work is available
     _audioJB->waitForWork();
     Work_ptr work=_audioJB->get();
     if(work==NULL) continue;

     if(work->is_finished){
        return;
     }

     doSendAudio(_ctx,work->buffer, work->len);

     _ctx->lastAudioTimestamp=work->timestamp;
  }
}
void AgoraIo::agora_disconnect(agora_context_t** ctx){

  logMessage("start agora disonnect ...");

  auto tempCtx=(*ctx);

  tempCtx->isRunning=false;
   //tell the thread that we are finished
    Work_ptr work=std::make_shared<Work>(nullptr,0, false);
    work->is_finished=true;

   tempCtx->videoJB->add(work);
   if(tempCtx->enable_dual){
       tempCtx->videoQueueLow->clear();
       tempCtx->videoQueueLow->add(work);
   }
   tempCtx->audioJB->add(work);

   std::this_thread::sleep_for(std::chrono::seconds(2));

   _connection->getLocalUser()->unpublishAudio(tempCtx->audioTrack);
   _connection->getLocalUser()->unpublishVideo(tempCtx->videoTrack);

   bool  isdisconnected=_connection->disconnect();
   if(isdisconnected){
      return;
   }


   tempCtx->audioSender = nullptr;
   tempCtx->videoSender = nullptr;
   tempCtx->audioTrack = nullptr;
   tempCtx->videoTrack = nullptr;


   tempCtx->videoJB=nullptr;;
   tempCtx->videoQueueLow=nullptr;
   tempCtx->audioJB=nullptr;;

   //delete context
   _connection=nullptr;

   _service->release();
   _service = nullptr;
  
   tempCtx->videoThreadHigh->detach();
   if(tempCtx->enable_dual){
      tempCtx->videoThreadLow->detach();
   }
   tempCtx->audioThread->detach();

   tempCtx->videoThreadHigh=nullptr;;
   tempCtx->videoThreadLow=nullptr;
   tempCtx->audioThread=nullptr; 

   tempCtx->videoDecoder=nullptr;
   tempCtx->videoEncoder=nullptr;

   delete (*ctx);

   logMessage("Agora disonnected ");
}

void agora_set_log_function(agora_context_t* ctx, agora_log_func_t f, void* log_ctx){

    ctx->log_func=f;
    ctx->log_ctx=log_ctx;
}

void agora_log_message(agora_context_t* ctx, const char* message){

   if(ctx->callConfig->useDetailedAudioLog()){
      logMessage(std::string(message));
   }
}

void agora_dump_audio_to_file(agora_context_t* ctx, unsigned char* data, short sampleCount)
{
    if(ctx->callConfig->dumpAudioToFile()==false){
       return;
    }

   std::ofstream meidaFile(ctx->audioDumpFileName, std::ios::binary|std::ios::app);	
   meidaFile.write(reinterpret_cast<const char*>(data), sampleCount*sizeof(float)); 
   meidaFile.close();
}

 size_t get_next_video_frame(agora_receive_context_t* context, 
                           unsigned char* data, size_t max_buffer_size,
                           int* is_key_frame){
                              
    if(context==NULL || context->_receiver==nullptr)  return 0;

    return context->_receiver->getNextVideoFrame(data, max_buffer_size, is_key_frame);;
 }

 size_t get_next_audio_frame(agora_receive_context_t* context, 
                                     unsigned char* data, size_t max_buffer_size){

      
   if(context==NULL || context->_receiver==nullptr)  return 0;

    return context->_receiver->getNextAudioFrame(data, max_buffer_size);

}

