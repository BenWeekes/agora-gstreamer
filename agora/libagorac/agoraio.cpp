#include "agoraio.h"
#include "helpers/utilities.h"

#include "agoralite.h"

#include <stdbool.h>
#include <fstream>

#include <string>
#include <cstring>
#include <iostream>
#include <fstream>

#include <sstream>
#include <list>

#include "syncbuffer.h"

#include "helpers/agoradecoder.h"
#include "helpers/agoraencoder.h"
#include "helpers/agoralog.h"
#include "helpers/localconfig.h"


AgoraIo::AgoraIo(const bool& verbose,
                event_fn fn,
			    void* userData,
                const int& in_audio_delay,
                const int& in_video_delay,
                const int& out_audio_delay,
                const int& out_video_delay,
                const bool& sendOnly,
                const bool& enableProxy,
                const int& proxyTimeout,
                const std::string& proxyIps,
                const bool& enableTranscode ):
 _verbose(verbose),
 _lastReceivedFrameTime(Now()),
 _currentVideoUser(""),
 _lastVideoUserSwitchTime(Now()),
 _isRunning(false),
 _isPaused(false),
 _eventfn(fn),
 _outSyncBuffer(nullptr),
 _inSyncBuffer(nullptr),
 _userEventData(userData),
 _in_audio_delay(in_audio_delay),
 _in_video_delay(in_video_delay),
 _out_audio_delay(out_audio_delay),
 _out_video_delay(out_video_delay),
 _videoOutFn(nullptr),
 _audioOutFn(nullptr),
 _lastTimeAudioReceived(Now()),
 _lastTimeVideoReceived(Now()),
 _isPublishingAudio(false),
 _isPublishingVideo(false),
 _videoOutFps(0),
 _videoInFps(0),
 _lastFpsPrintTime(Now()),
 _sendOnly(sendOnly),
 _lastSendTime(Now()),
 _enableProxy(enableProxy),
 _proxyConnectionTimeOut((proxyTimeout < 1 ) ? 10000 : proxyTimeout),
 _proxyIps(proxyIps),
 _transcodeWidth(0),
 _transcodeHeight(0),
 _transcodeWidthLow(640),
 _transcodeHeightLow(360),
 _transcodeWidthMedium(1280),
 _transcodeHeightMedium(720),
 _requireTranscode(true), // start off with transcoder
 _requireKeyframe(false),
_transcodeVideo(enableTranscode)
{

   _activeUsers.clear();
}

class Work{

public:

   Work(const unsigned char* b, const unsigned long& l, bool is_key):
	buffer(nullptr){
  
       if(b==nullptr){
	       return;
       }

       buffer=new unsigned char[l];
       memcpy(buffer, b, l);

       len=l;
    
       is_key_frame=is_key;
       is_finished=0;
   }

   ~Work(){
     if(buffer==nullptr) return;
     delete [] buffer;
   }

   unsigned char*        buffer;
   unsigned long         len;
   bool                  is_key_frame;

   bool                  is_finished;

   uint64_t               timestamp;
   
};

int calcVol(const int16_t* samples, const uint16_t& packetLen){
	
       int32_t sum=0;
       for(int i=0;i<packetLen;i++)	
         sum +=std::abs(samples[i]);
	
       return sum/(double)packetLen;
}

#define ENC_KEY_LENGTH        128

void onVideoFrameReceived(u_int64_t user_id,
	                      const u_int8_t* buffer,
                          u_int64_t len,
						  bool   is_key_frame,
						  void* user_data){

    AgoraIo* agoraIo=static_cast<AgoraIo*>(user_data);
    if(agoraIo==nullptr){
        return;
    }

    agoraIo->receiveVideoFrame(user_id, buffer, len, is_key_frame, 0);

}

void onAudioFrameReceived(u_int64_t user_id,
	                      const u_int8_t* buffer,
                          u_int64_t len,
						  bool   is_key_frame,
						  void* user_data){

    AgoraIo* agoraIo=static_cast<AgoraIo*>(user_data);
    if(agoraIo==nullptr){
        return;
    }

    agoraIo->receiveAudioFrame(user_id, buffer, len, 0);

}

void onAgoraEvent(const int& eventType, 
                  const char* userName,
                  const long& param1, 
                  const long& param2,
				  void* user_data){

    AgoraIo* agoraIo=static_cast<AgoraIo*>(user_data);
    if(agoraIo==nullptr){
        return;
    }

    agoraIo->addEvent((AgoraEventType)(eventType),userName,param1,param2);
    /*if(param1==USER_STATE_CAM_OFF){
        agoraIo->handleUserStateChange(userName, USER_STATE_CAM_OFF);
    }
    else if(param1==USER_STATE_CAM_ON){
        agoraIo->handleUserStateChange(userName, USER_STATE_CAM_ON);
    }*/
}

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

    g_context_t * l_ctx=agora_lite_init(in_app_id, in_ch_id, in_user_id);
	if(l_ctx==nullptr){
		return false;
	}

    l_ctx->is_send_only=_sendOnly;
    l_ctx->channel_id=in_ch_id;

    //set video and audio receive callback
    l_ctx->video_callback_fn=onVideoFrameReceived;
    l_ctx->video_user_data=this;

    //set video and audio receive callback
    l_ctx->audio_callback_fn=onAudioFrameReceived;
    l_ctx->audio_user_data=this;

    //set event callback
    l_ctx->event_callback_fn=onAgoraEvent;
    l_ctx->event_user_data=this;
    

    //TODO: we have to check if this version of the sdk supports proxy or not

    // Publish  video and audio tracks
    startPublishAudio();
    startPublishVideo();

    //initialize encoder and decoder
    if(!initTranscoder()){
        std::cout<<"failed to init transcoder\n";
    }
    
    /*
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
    });*/

  //setup the out sync buffer (source -> AG sdk)
  _outSyncBuffer=std::make_shared<SyncBuffer>(_in_video_delay, _in_audio_delay, false);
  _outSyncBuffer->setVideoOutFn([this](const uint8_t* buffer,
                                         const size_t& bufferLength,
                                         const bool& isKeyFrame){

        doSendHighVideo(buffer, bufferLength, isKeyFrame);

    });

    _outSyncBuffer->setAudioOutFn([this](const uint8_t* buffer,
                                         const size_t& bufferLength){
          doSendAudio(buffer, bufferLength);
    });

    _outSyncBuffer->start();

    //setup the in sync buffer ( AG sdk -> source)
    _inSyncBuffer=std::make_shared<SyncBuffer>(_out_video_delay, _out_audio_delay, false);
    _inSyncBuffer->setVideoOutFn([this](const uint8_t* buffer,
                                         const size_t& bufferLength,
                                         const bool& isKeyFrame){
        if(_videoOutFn!=nullptr){
            _videoOutFn(buffer, bufferLength, _videoOutUserData);
            _videoInFps++;
        }

    });

    _inSyncBuffer->setAudioOutFn([this](const uint8_t* buffer,
                                         const size_t& bufferLength){

        
        if(_audioOutFn!=nullptr){
            _audioOutFn(buffer, bufferLength, _audioOutUserData); 
        } 
          
    });

    _inSyncBuffer->start();

    _isRunning=true;
    _connected = true;

    _publishUnpublishCheckThread=std::make_shared<std::thread>(&AgoraIo::publishUnpublishThreadFn,this);
    _publishUnpublishCheckThread->detach();

    return _connected;
}


void AgoraIo::receiveVideoFrame(const uint userId, 
                                const uint8_t* buffer,
                                const size_t& length,
                                const int& isKeyFrame,
                                const uint64_t& ts){

    //do not read video if the pipeline is in pause state
    if(_isPaused) return;

    if(_inSyncBuffer!=nullptr && _isRunning){
        _inSyncBuffer->addVideo(buffer, length, isKeyFrame, ts);
    }
}

void AgoraIo::receiveAudioFrame(const uint userId, 
                                const uint8_t* buffer,
                                const size_t& length,
                                const uint64_t& ts){

    //do not read audio if the pipeline is in pause state
    if(_isPaused ) return;

     if(_inSyncBuffer!=nullptr && _isRunning){
        _inSyncBuffer->addAudio(buffer, length, ts);
     } 
}

/*void AgoraIo::handleUserStateChange(const std::string& userId, 
                                     const UserState& newState){

    if(newState==USER_JOIN){
        subscribeAudioUser(userId);
        _pcmFrameObserver->setUserJoined(true);
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
}*/

 void AgoraIo::subscribeToVideoUser(const std::string& userId){

    /*if(_sendOnly==false){  
        
#if SDK_BUILD_NUM==190534
        agora::rtc::VideoSubscriptionOptions subscriptionOptions;
#else
        agora::rtc::ILocalUser::VideoSubscriptionOptions subscriptionOptions;
#endif
        subscriptionOptions.encodedFrameOnly = true;
        subscriptionOptions.type = agora::rtc::VIDEO_STREAM_HIGH;
        _connection->getLocalUser()->subscribeVideo(userId.c_str(), subscriptionOptions);

        _currentVideoUser=userId;
        std::cout<<"subscribed to video user #"<<_currentVideoUser<<std::endl;

        addEvent(AGORA_EVENT_ON_VIDEO_SUBSCRIBED,userId,0,0);
    }*/
 }


size_t AgoraIo::getNextAudioFrame(uint8_t* data, size_t max_buffer_size){
   
    const int MS_PER_AUDIO_PACKET=10;


    _receivedAudioFrames->waitForWork();
    Work_ptr work=_receivedAudioFrames->get();

    memcpy(data, work->buffer, work->len);

    return work->len;
}

void AgoraIo::subscribeAudioUser(const std::string& userId){

   // _connection->getLocalUser()->subscribeAudio(userId.c_str());
    std::cout<<"subscribed to audio user "<<userId<<std::endl;
}
void AgoraIo::unsubscribeAudioUser(const std::string& userId){

  //_connection->getLocalUser()->unsubscribeAudio(userId.c_str());
}



int AgoraIo::isIFrame(const uint8_t* buffer, uint64_t len) {

  int fragment_type = buffer[0] & 0x1F;
  int nal_type = buffer[1] & 0x1F;
  int start_bit = buffer[1] & 0x80;

  /*
  std::cout << "  \n ";
  for(int i=0;i<20;i++) {
    std::cout<< (buffer[i]) << " ";
  }
  std::cout << " \n";
  std::cout << " 1F \n ";
  for(int i=0;i<20;i++) {
    std::cout<< (buffer[i] & 0x1F) << " ";
  }
  std::cout << " \n";
  std::cout << " 3F \n ";
  for(int i=0;i<20;i++) {
    std::cout<< (buffer[i] & 0x3F) << " ";
  }
  std::cout << " \n";
  std::cout << " 7F \n ";
  for(int i=0;i<20;i++) {
    std::cout<< (buffer[i] & 0x7F) << " ";
  }
  std::cout << " \n";
  std::cout << " FF \n ";
  */
 // for(int i=0;i<100;i++) {
  //  std::cout<< (buffer[i] & 0xFF) << " ";
  //}
  //
  //std::cout << " \n";

 // std::cout << " \n";



  if (((fragment_type == 28 || fragment_type == 29) && nal_type == 5 && start_bit == 128) || fragment_type == 5){
    return 1;
  }

  if (len>10 && (buffer[10] & 0x1F) == 7 ) {
	  return 1;
  }

  if (len>45 && (buffer[43] & 0xFF) == 1 && (buffer[44] & 0xFF) == 103 && (buffer[45] & 0xFF) == 100) {
	  return 1;
  }

  //if (len > 50000) // studio hack for iframe
//	  return 1;
  return 0;
}

bool AgoraIo::doSendHighVideo(const uint8_t* buffer,  uint64_t len,int is_key_frame){

  int is_iframe=isIFrame(buffer, len);

  agora_lite_send_video(buffer, len, is_key_frame, 0);

  return true;
}

bool AgoraIo::initTranscoder(){

     if (!_transcodeVideo) return true;

    _videoDecoder=std::make_shared<AgoraDecoder>();
    if(_videoDecoder->init()){
        std::cout<<"decoder is initialized successfully\n";
    }

    int bitrate=300000, fps=30;
    _transcodeWidth=_transcodeWidthLow;
    _transcodeHeight=_transcodeHeightLow;
    _videoEncoder=std::make_shared<AgoraEncoder>(_transcodeWidth,_transcodeHeight,bitrate,fps);

    if(_videoEncoder->init()){
         std::cout<<"encoder is  initialized successfully\n";
    }

    return true;
}

bool AgoraIo::setTranscoderProps(int width, int height, int bitrate, int fps){

  if (_transcodeWidth!=width || _transcodeHeight!=height) {
  	if (_videoEncoder->propsChange(width, height, bitrate ,fps)) {
		_transcodeWidth=width;
		 _transcodeHeight=height;
	  	std::cout<<"encoder change w=" << width << " h="  << height << " br=" << bitrate << " fps=" << fps << " \n";
  	}
  } else {
	 _videoEncoder->bitrateChange(bitrate);
	 std::cout<<"encoder change br=" << bitrate << " \n";
  }

    return true;
}

void AgoraIo::setTranscoderResolutions(int width, int height){

  if (_transcodeWidthLow==(width/4))
	  return;

  _transcodeWidthLow=width/4;
  _transcodeHeightLow=height/4;

  _transcodeWidthMedium=width/2;
  _transcodeHeightMedium=height/2;

  setTranscoderProps(_transcodeWidthLow, _transcodeHeightLow, 300000, 30);

}

uint32_t AgoraIo::transcode(const uint8_t* inBuffer,  const uint64_t& inLen,
                             uint8_t* outBuffer, bool isKeyFrame){

    uint32_t outBufferSize=0;
    if(_videoDecoder->decode(inBuffer, inLen)){
        auto frame=_videoDecoder->getFrame();
	if (frame->height>0) {
		setTranscoderResolutions(frame->width,frame->height);
	}
	// make sure encoder aspect ration correct
	// std::cout<<" frame-width " << frame->width << " \n";
	// std::cout<<" frame-height " << frame->height << " \n";
        _videoEncoder->encode(frame, outBuffer, outBufferSize,isKeyFrame);
    }

  return outBufferSize;
}

bool AgoraIo::doSendAudio(const uint8_t* buffer,  uint64_t len){

    agora_lite_send_audio(buffer, len);

  return true;
}

int AgoraIo::sendVideo(const uint8_t * buffer,  
                              uint64_t len,
                              int is_key_frame,
                              long timestamp){

    //do nothing if we are in pause state
    if(_isPaused==true){
        return 0;
    }

    if(_outSyncBuffer!=nullptr && _isRunning){
         startPublishVideo();
        _outSyncBuffer->addVideo(buffer, len, is_key_frame, timestamp);
    }

    _lastTimeVideoReceived=Now();

    showFps();

   return 0; //no errors
}

void AgoraIo::showFps(){

   if(_verbose){
        _videoOutFps++;
        if(_lastFpsPrintTime+std::chrono::milliseconds(1000)<=Now()){

            std::cout<<"Out video fps: "<<_videoOutFps<<std::endl;
            std::cout<<"In video fps: "<<_videoInFps<<std::endl;

            _videoInFps=0;
            _videoOutFps=0;

            _lastFpsPrintTime=Now();
        }
    }
}

int AgoraIo::sendAudio(const uint8_t * buffer,  
                       uint64_t len,
                       long timestamp,
                       const long& duration){

    TimePoint nextAudiopacketTime=_lastTimeAudioReceived+std::chrono::milliseconds(duration);

    //do nothing if we are in pause state
    if(_isPaused==true){
        return 0;
    }

    if(_outSyncBuffer!=nullptr && _isRunning){

        startPublishAudio();
        _outSyncBuffer->addAudio(buffer, len, timestamp);
     }

    //block this thread loop until the duration of the current buffer is elapsed
    if(duration>0){
        std::this_thread::sleep_until(nextAudiopacketTime);
    }

    _lastTimeAudioReceived=Now();

    return 0;
}
void AgoraIo::disconnect(){

    logMessage("start agora disonnect ...");

   _isRunning=false;

   //tell the thread that we are finished
    _outSyncBuffer->stop();
    _inSyncBuffer->stop();

   std::this_thread::sleep_for(std::chrono::seconds(2));

   agora_lite_disconnect();


   _outSyncBuffer=nullptr;
   _inSyncBuffer=nullptr;
   

   std::cout<<"agora disconnected\n";

   logMessage("Agora disonnected ");
}

void agora_log_message(const char* message){

   //if(ctx->callConfig->useDetailedAudioLog()){
     // logMessage(std::string(message));
   //}
}

void AgoraIo::unsubscribeAllVideo(){

    //_connection->getLocalUser()->unsubscribeAllVideo(); 
}
void AgoraIo::setPaused(const bool& flag){

    _isPaused=flag;
    if(_isPaused==true){
        unsubscribeAllVideo();

        stopPublishVideo();
        stopPublishAudio();
    }
    else{

      //clear any buffering 
      _inSyncBuffer->clear();
      _outSyncBuffer->clear();

      startPublishVideo();
      startPublishAudio();

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

 void AgoraIo::setVideoOutFn(agora_media_out_fn videoOutFn, void* userData){
     _videoOutFn=videoOutFn;
     _videoOutUserData=userData;
 }

void AgoraIo::setAudioOutFn(agora_media_out_fn videoOutFn, void* userData){
     _audioOutFn=videoOutFn;
     _audioOutUserData=userData;
 }

 void AgoraIo::publishUnpublishThreadFn(){

     std::cout<<"Agoraio: publish/unpublish thread started\n";

     long checkTimeMs=500;
      while(_isRunning){

         long allowedUnpublishedTime=1000; //ms
         if((_lastTimeAudioReceived+std::chrono::milliseconds(allowedUnpublishedTime))<Now()){
              stopPublishAudio();
          }

         if((_lastTimeVideoReceived+std::chrono::milliseconds(allowedUnpublishedTime))<Now()){
            stopPublishVideo();
         }

         TimePoint  nextCheckTime = Now()+std::chrono::milliseconds(checkTimeMs);
         std::this_thread::sleep_until(nextCheckTime);

     }
 }

void AgoraIo::startPublishAudio(){

    if(_isPublishingAudio==true){
        return;
    }

    _isPublishingAudio=true;

    std::cout<<"Agoraio: published Audio\n";

 }
void AgoraIo::startPublishVideo(){

    if(_isPublishingVideo==true){
        return;
    }
    _isPublishingVideo=true;

    std::cout<<"Agoraio: published video\n";
}

void AgoraIo::stopPublishAudio(){

    if(_isPublishingAudio==false){
        return;
    }
    _isPublishingAudio=false;

    std::cout<<"Agoraio: unpublished Audio\n";
}
void AgoraIo::stopPublishVideo(){

    if(_isPublishingVideo==false){
        return;
    }

    _isPublishingVideo=false;

    std::cout<<"Agoraio: unpublished video\n";
}

void AgoraIo::setSendOnly(const bool& flag){
    _sendOnly=flag;
}

std::list<std::string> AgoraIo::parseIpList(){

    std::stringstream ss(_proxyIps);

   std::list<std::string>  returnList;
   returnList.clear();

    while (ss.good()){
        std::string ip;
        getline(ss, ip, ',');
        returnList.emplace_back(ip);

        std::cout<<ip<<std::endl;
    }

  return returnList;
}

std::string AgoraIo::createProxyString(std::list<std::string> ipList){

    //TODO: this is a reference of how the proxy string looks like
    //agoraParameter->setParameters("{\"rtc.proxy_server\":[2, \"[\\\"128.1.77.34\\\", \\\"128.1.78.146\\\"]\", 0], \"rtc.enable_proxy\":true}");

    std::string ipListStr="\"[";
    bool addComma=false;
    for(const auto ip: ipList){

        if(addComma){
            ipListStr+=",";
        } 
        else{
            addComma=true;
        }

        ipListStr +="\\\""+ip+"\\\" ";
    }

    ipListStr+="]\", ";

    std::string proxyString="{\"rtc.proxy_server\":[2, "+
                             ipListStr +
                             "0], \"rtc.enable_proxy\":true}";

    return proxyString;
}
