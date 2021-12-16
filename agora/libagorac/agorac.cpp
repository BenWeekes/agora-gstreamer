#include <stdbool.h>
#include <fstream>
#include "agorac.h"

#include <string>
#include <cstring>
#include <iostream>
#include <fstream>


//agora header files
#include "NGIAgoraRtcConnection.h"
#include "IAgoraService.h"
#include "AgoraBase.h"

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

#include "agoraio.h"


//threads
static void VideoThreadHandlerHigh(agora_context_t* ctx);
static void VideoThreadHandlerLow(agora_context_t* ctx);
static void AudioThreadHandler(agora_context_t* ctx);


//do not use it before calling agora_init
void agora_log(agora_context_t* ctx, const char* message){
   ctx->log_func(ctx->log_ctx, message);
}

//helper function for creating a service
agora::base::IAgoraService* createAndInitAgoraService(bool enableAudioDevice,
                                                      bool enableAudioProcessor,
						      bool enableVideo,
						      bool stringUserid,
						      bool enableEncryption, const char* appid) {
  auto service = createAgoraService();
  agora::base::AgoraServiceConfiguration scfg;
  scfg.enableAudioProcessor = enableAudioProcessor;
  scfg.enableAudioDevice = enableAudioDevice;
  scfg.enableVideo = enableVideo;
  scfg.useStringUid=stringUserid;
  if (enableEncryption) {
    scfg.appId = appid;
  }

  int ret = service->initialize(scfg);
  return (ret == agora::ERR_OK) ? service : nullptr;
}


#define ENC_KEY_LENGTH        128
agora_context_t*  agora_init(char* in_app_id, char* in_ch_id, char* in_user_id,
                              bool is_audiouser,
                              bool enable_enc,
		                         short enable_dual, unsigned int  dual_vbr, 
			                       unsigned short  dual_width, unsigned short  dual_height,
                             unsigned short min_video_jb, unsigned short dfps){

  int32_t buildNum = 0;
  getAgoraSdkVersion(&buildNum);
  logMessage("**** Agora SDK version: "+std::to_string(buildNum)+" ****");

  //rotate log file if necessary
  CheckAndRollLogFile();

  agora_context_t* ctx=new agora_context_t;

  std::string app_id(in_app_id);
  std::string chanel_id=(in_ch_id);
  std::string user_id(in_user_id);
  std::string proj_appid = "abcd";
  char encryptionKey[ENC_KEY_LENGTH] = "";

  //set configuration
  ctx->config.clientRoleType = agora::rtc::CLIENT_ROLE_BROADCASTER;
  ctx->config.channelProfile = agora::CHANNEL_PROFILE_LIVE_BROADCASTING;
  ctx->config.autoSubscribeAudio = false;
  ctx->config.autoSubscribeVideo = false;

  ctx->enable_dual=enable_dual;

  //create a local config
  ctx->callConfig=std::make_shared<LocalConfig>();
  ctx->callConfig->loadConfig("/usr/local/nginx/conf/rtmpg.conf");
  ctx->callConfig->print();

  ctx->jb_size=ctx->callConfig->getInitialJbSize();

  if (enable_enc) {
    std::ifstream inf;
    std::string temp_str;
    inf.open("/tmp/nginx_agora_appid.txt",std::ifstream::in);
    if(!inf.is_open()){
      logMessage("agora Failed to open AppId and key for encryption!");
    }
    getline(inf, proj_appid);
    //logMessage(proj_appid.c_str());
    getline(inf, temp_str);
    //logMessage(temp_str.c_str());
    inf.close();
    int str_leng = temp_str.copy(encryptionKey, temp_str.length());
    encryptionKey[str_leng] = '\0';
  }

  // Create Agora service
   if(user_id=="" ||  isNumber(user_id)){
      logMessage("numeric  user id: "+ user_id);
      ctx->service = createAndInitAgoraService(false, true, true,  false, enable_enc, proj_appid.c_str());
  }
  else{
     logMessage("string user id: "+ user_id);
     ctx->service = createAndInitAgoraService(false, true, true,  true, enable_enc, proj_appid.c_str());
  }


  if (!ctx->service) {
    delete ctx;
    return NULL;
  }

  ctx->connection =ctx->service->createRtcConnection(ctx->config);
  if (!ctx->connection) {
     delete ctx;
     return NULL;
  }

  // open the encryption mode
  if ( ctx->connection && enable_enc) {
    agora::rtc::EncryptionConfig Config;
    Config.encryptionMode = agora::rtc::SM4_128_ECB;  //currently only this mode is supported
    Config.encryptionKey = encryptionKey;

    if (ctx->connection->enableEncryption(true, Config) < 0) {
      logMessage("agora Failed to enable Encryption!");
      delete ctx;
      return NULL;
    } else {
      logMessage("agora built-in encryption enabled!");
    }
  }

   // Register connection observer to monitor connection event
   ctx->_connectionObserver = std::make_shared<ConnectionObserver>(nullptr);
   ctx->connection->registerObserver(ctx->_connectionObserver.get());

    ctx->_connectionObserver->setOnUserConnected([is_audiouser](const std::string& userId, const UserState& newState){

         if(is_audiouser==true){
            UidToFile uidfile;
            uidfile.writeUid(userId);
         }
    });

  // Connect to Agora channel
  auto  connected =  ctx->connection->connect(app_id.c_str(), chanel_id.c_str(), user_id.c_str());
  if (connected) {
     delete ctx;
     return NULL;
  }

  //ctx->localUserObserver = std::make_shared<MyUserObserver>(ctx->connection->getLocalUser());

  // Create media node factory
  auto factory = ctx->service->createMediaNodeFactory();
  if (!factory) {
    return NULL;
  }


  //audio
  // Create audio data sender
   ctx->audioSender = factory->createAudioEncodedFrameSender();
  if (!ctx->audioSender) {
    return NULL;
  }

  // Create audio track
  ctx->audioTrack =ctx->service->createCustomAudioTrack(ctx->audioSender, agora::base::MIX_DISABLED);
  if (!ctx->audioTrack) {
    return NULL;
  }

  // Create video frame sender
  ctx->videoSender = factory->createVideoEncodedImageSender();
  if (!ctx->videoSender) {
    return NULL;
  }


  // Create video track
  agora::base::SenderOptions options;
  options.ccMode=agora::base::CC_ENABLED;  //for send_dual_h264
  ctx->videoTrack =ctx->service->createCustomVideoTrack(ctx->videoSender,options);
  if (!ctx->videoTrack) {
    return NULL;
  }

  // Set the dual_model
  agora::rtc::SimulcastStreamConfig Low_streamConfig;
  ctx->videoTrack->enableSimulcastStream(true, Low_streamConfig);

  // Publish audio & video track
  ctx->connection->getLocalUser()->publishAudio(ctx->audioTrack);
  ctx->connection->getLocalUser()->publishVideo(ctx->videoTrack);

  ctx->isConnected=1;
  
  ctx->videoJB=std::make_shared<WorkQueue <Work_ptr> >();
  ctx->audioJB=std::make_shared<WorkQueue <Work_ptr> >();

  ctx->isRunning=true;

  //start thread handlers
  ctx->videoThreadHigh=std::make_shared<std::thread>(&VideoThreadHandlerHigh,ctx);
  ctx->audioThread=std::make_shared<std::thread>(&AudioThreadHandler,ctx);
 
  //is dual streaming is enabled 
  if(ctx->enable_dual){
      //create video encoder/decoder
      ctx->videoDecoder=std::make_shared<AgoraDecoder>();
      ctx->videoEncoder=std::make_shared<AgoraEncoder>(dual_width,dual_height,dual_vbr, dfps);

      ctx->videoEncoder->setQMin(ctx->callConfig->getQMin());
      ctx->videoEncoder->setQMax(ctx->callConfig->getQMax());

      if(!ctx->videoDecoder->init() || ! ctx->videoEncoder->init()){
         return NULL;
      }

      ctx->videoQueueLow=std::make_shared<WorkQueue <Work_ptr> >();
      ctx->videoThreadLow=std::make_shared<std::thread>(&VideoThreadHandlerLow,ctx);
  }


  ctx->encodeNextFrame=true;

  ctx->fps=0;
  ctx->lastFpsPrintTime=Now();
  ctx->lastBufferingTime=Now();
  ctx->reBufferingCount=0;

  ctx->lastFrameTimestamp=0; 
  ctx->timestampPerSecond=0;

  //initially set to 30fps
  ctx->predictedFps=30;

  ctx->lastHighFrameSendTime=Now();
  ctx->lastLowFrameSendTime=Now();

  ctx->isJbBuffering=false;

  ctx->lastVideoTimestamp=0;
  ctx->lastAudioTimestamp=0;

  ctx->highVideoFrameCount=0;
  ctx->lowVideoFrameCount=0;

  ctx->lastVideoSampingInterval=1000/(float)ctx->predictedFps;

  ctx->dfps=dfps;

  ctx->audioDumpFileName="/tmp/rtmp_agora_audio_"+std::to_string((long)(ctx))+".raw";

  return ctx;
}

agora_receive_context_t* agora_receive_init(char* app_id, 
                                            char* ch_id,
                                            char* user_id,
                                            int receiveAudio,
													     int receiveVideo,
                                            int verbose, 
                                            char* filePath){
    
      std::shared_ptr<AgoraReceiverUser> receiver=std::make_shared<AgoraReceiverUser>(
                                          app_id, ch_id, user_id,
                                          receiveAudio, receiveVideo,
                                          verbose,
                                          filePath); 

    if(!receiver->connect()){
       return NULL;
    }

    agora_receive_context_t* context=new agora_receive_context_t;
    context->_receiver=receiver;

    return context;
}

bool doSendLowVideo(agora_context_t* ctx, const unsigned char* buffer,  unsigned long len,int is_key_frame){

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
    ctx->videoSender->sendEncodedVideoImage(outBuffer.get(),outBufferSize,videoEncodedFrameInfo);

    ctx->lowVideoFrameCount++;
  }

  return true;

}

bool doSendHighVideo(agora_context_t* ctx, const unsigned char* buffer,  unsigned long len,int is_key_frame){

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

  ctx->videoSender->sendEncodedVideoImage(buffer,len,videoEncodedFrameInfo);

  return true;
}

bool doSendAudio(agora_context_t* ctx, const unsigned char* buffer,  unsigned long len){

  agora::rtc::EncodedAudioFrameInfo audioFrameInfo;
  audioFrameInfo.numberOfChannels =1; //TODO
  audioFrameInfo.sampleRateHz = 48000; //TODO
  audioFrameInfo.codec = agora::rtc::AUDIO_CODEC_OPUS;

  ctx->audioSender->sendEncodedAudioFrame(buffer,len, audioFrameInfo);

  return true;
}

void UpdatePredictedFps(agora_context_t* ctx, const long& timestamp){

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

int agora_send_video(agora_context_t* ctx,
                    const unsigned char * buffer,  
                    unsigned long len,
                    int is_key_frame,
                    long timestamp){

   auto     timePerFrame=(1000/(float)ctx->predictedFps);
   Work_ptr work=std::make_shared<Work>(buffer,len, is_key_frame);
   work->timestamp=timestamp;
   
   ctx->videoJB->add(work);
 
   //update fps based on frame timestamp
   UpdatePredictedFps(ctx, timestamp);
  
   //log current fps
   if(GetTimeDiff(ctx->lastFpsPrintTime, Now())>=1000){

      if(ctx->callConfig->useFpsLog() ){
         logMessage(GetAddressAsString(ctx)+"JB-Video: (ms): " +
                           std::to_string((int)(ctx->videoJB->size()*timePerFrame)) +
                            "/"+
                           std::to_string(ctx->jb_size) +
                           ", fps="+std::to_string(ctx->fps)+
                           ", predicted fps="+ std::to_string(ctx->predictedFps));
     }

     ctx->highVideoFrameCount=0;
     ctx->lowVideoFrameCount=0;

     ctx->lastFpsPrintTime=Now();
     ctx->fps=0;

   }

   return 0; //no errors
}

int agora_send_audio(agora_context_t* ctx,
                     const unsigned char * buffer, 
                     unsigned long len,
                     long timestamp){

    Work_ptr work=std::make_shared<Work>(buffer,len, 0);
    work->timestamp=timestamp;

    ctx->audioJB->add(work);

    return 0;
}

int agoraio_send_audio(AgoraIoContext_t* ctx,
                     const unsigned char * buffer, 
                     unsigned long len,
                     long timestamp){

    Work_ptr work=std::make_shared<Work>(buffer,len, 0);
    work->timestamp=timestamp;

    if(ctx->agoraIo!=nullptr){
       ctx->agoraIo->addAudioFrame(work);
     }

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
void CheckAndFillInVideoJb(agora_context_t* ctx, const TimePoint& lastSendTime){

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

static void VideoThreadHandlerHigh(agora_context_t* ctx){

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

static void VideoThreadHandlerLow(agora_context_t* ctx){

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

    //logMessage("LOW video timestamp: "+std::to_string(work->timestamp)+", delta: "+std::to_string(delta));
  }

  logMessage("VideoThreadHandlerLow ended");
}

static void AudioThreadHandler(agora_context_t* ctx){

   const int waitTimeForBufferToBeFull=10;
   
   while(ctx->isRunning==true){

     //TimePoint  nextSample = GetNextSamplingPoint(ctx,48, ctx->audioJB);

     //wait untill work is available
     ctx->audioJB->waitForWork();
     Work_ptr work=ctx->audioJB->get();
     if(work==NULL) continue;

     if(work->is_finished){
        return;
     }

     /*TimePoint  nextSample =
          GetNextSamplingPoint("Audio: ", ctx,ctx->audioJB,work->timestamp, ctx->lastAudioTimestamp);*/

     //when video JB is buffering, audio should buffer too
    /* while(ctx->isJbBuffering && ctx->isRunning){
        std::this_thread::sleep_for(std::chrono::milliseconds(waitTimeForBufferToBeFull));
     }*/

     doSendAudio(ctx,work->buffer, work->len);

     ctx->lastAudioTimestamp=work->timestamp;

     //logMessage("AUDIO timestamp: "+std::to_string(work->timestamp));

    // std::this_thread::sleep_until(nextSample);
  }
}
void agora_disconnect(agora_context_t** ctx){

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

   tempCtx->connection->getLocalUser()->unpublishAudio(tempCtx->audioTrack);
   tempCtx->connection->getLocalUser()->unpublishVideo(tempCtx->videoTrack);

   bool  isdisconnected=tempCtx->connection->disconnect();
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
   tempCtx->connection=nullptr;

   tempCtx->service->release();
   tempCtx->service = nullptr;
  
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

 size_t agoraio_read_video(AgoraIoContext_t* ctx, 
                           unsigned char* data, 
                           size_t max_buffer_size,
                           int* is_key_frame,
                           u_int64_t* ts){
                              
    if(ctx==NULL || ctx->agoraIo==nullptr)  return 0;

    return ctx->agoraIo->getNextVideoFrame(data, max_buffer_size, is_key_frame,ts);
 }

 size_t agoraio_read_audio(AgoraIoContext_t* ctx, 
                                  unsigned char* data, size_t max_buffer_size){

   if(ctx==NULL ||  ctx->agoraIo==nullptr)  return 0;

    return ctx->agoraIo->getNextAudioFrame(data, max_buffer_size);

}

 size_t get_next_audio_frame(agora_receive_context_t* context, 
                                     unsigned char* data, size_t max_buffer_size){

      
   if(context==NULL || context->_receiver==nullptr)  return 0;

    return context->_receiver->getNextAudioFrame(data, max_buffer_size);

}

AgoraIoContext_t*  agoraio_init2(char* app_id, char* ch_id, char* user_id,
                                        bool is_audiouser,
                                        bool enc_enable,
		                                  short enable_dual,
										          unsigned int    dual_vbr, 
				                            unsigned short  dual_width, 
										          unsigned short  dual_height,
									             unsigned short  min_video_jb,
										          unsigned short  dfps,
                                        bool verbose,
                                        event_fn fn,
										          void* userData){

    AgoraIoContext_t* ctx=new AgoraIoContext_t;
    if(ctx==nullptr){
        return NULL;
    }

    ctx->agoraIo=std::make_shared<AgoraIo>(verbose, fn, userData);

    ctx->agoraIo->init(app_id, ch_id,user_id,
                       is_audiouser, enc_enable, enable_dual,
                       dual_vbr, dual_width, dual_height,
                       min_video_jb, dfps);

    return ctx;

}

int  agoraio_send_video(AgoraIoContext_t* ctx,  
                                const unsigned char* buffer,  
							           unsigned long len, 
								        int is_key_frame,
							           long timestamp){

        return ctx->agoraIo->sendVideo( buffer, len, is_key_frame, timestamp);

}

void agoraio_disconnect(AgoraIoContext_t** ctx){

   if(ctx==nullptr){
      std::cout<<"cannot disconnect agora!\n";
   }
   (*ctx)->agoraIo->disconnect();

   delete (*ctx);
}

void logText(const char* message){
    logMessage(message);
}

void  agoraio_set_paused(AgoraIoContext_t* ctx, int flag){

    if(ctx==nullptr){
         return;
    }

    (ctx)->agoraIo->setPaused(flag);
}

void agoraio_set_event_handler(AgoraIoContext_t* ctx, event_fn fn, void* userData){

    if(ctx==nullptr)  return;

    ctx->agoraIo->setEventFunction(fn, userData);
}


