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
#include "helpers/agoradecoder.h"
#include "helpers/agoraencoder.h"
#include "helpers/utilities.h"
#include "agoratype.h"
#include "helpers/agoraconstants.h"

#include "helpers/uidtofile.h"

#include "syncbuffer.h"
#include <sstream>
#include <list>


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
  _userEventData(userData),
 _outSyncBuffer(nullptr),
 _inSyncBuffer(nullptr),
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
 _transcodeVideo(enableTranscode),
 _requireTranscode(true), // start off with transcoder
 _requireKeyframe(false),
 _transcodeWidth(0),
 _transcodeHeight(0),
 _transcodeWidthLow(640),
 _transcodeHeightLow(360),
 _transcodeWidthMedium(1280),
 _transcodeHeightMedium(720)
{

   _activeUsers.clear();
}

bool AgoraIo::initAgoraService(const std::string& appid)
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

    if(verifyLicense() != 0) {
      return false;
    }

    return true;
}

int calcVol(const int16_t* samples, const uint16_t& packetLen){
	
       int32_t sum=0;
       for(int i=0;i<packetLen;i++)	
         sum +=std::abs(samples[i]);
	
       return sum/(double)packetLen;
}

#define ENC_KEY_LENGTH        128

bool AgoraIo::doConnect(char* in_app_id,
                        char* in_channel_id,
                        char* in_user_id){


	 _connection->connect(in_app_id, in_channel_id, in_user_id);
	return true;
}

bool AgoraIo::checkConnection(){

     auto connectionInfo=_connection->getConnectionInfo();
     if(connectionInfo.state==agora::rtc::CONNECTION_STATE_CONNECTED){
         return true;
     }
     
    if(_connectionObserver!=nullptr){
         _connectionObserver->waitUntilConnected(_proxyConnectionTimeOut);
    }
    else{
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    connectionInfo=_connection->getConnectionInfo();
    if(connectionInfo.state==agora::rtc::CONNECTION_STATE_CONNECTED){
         return true;
     }
     return false;
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

    if(!initAgoraService(in_app_id)){
        return false;
    }


    std::string _userId=in_user_id;
    
    _rtcConfig.clientRoleType = agora::rtc::CLIENT_ROLE_BROADCASTER;
    // set to LIVE but SDK 3.4.1 is only able to do LIVE
    _rtcConfig.channelProfile = agora::CHANNEL_PROFILE_LIVE_BROADCASTING;
    //_rtcConfig.channelProfile = agora::CHANNEL_PROFILE_COMMUNICATION;
   //_rtcConfig.channelProfile = agora::CHANNEL_PROFILE_COMMUNICATION_1v1;
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
    _yuvFrameObserver = std::make_shared<YuvFrameObserver>();
    if (_sendOnly==false && _connection->getLocalUser()->setPlaybackAudioFrameParameters(1, 48000) != 0) {
        logMessage("Agora: Failed to set audio frame parameters!");
        return false;
    }

    if (_sendOnly==false && _connection->getLocalUser()->setPlaybackAudioFrameBeforeMixingParameters(1, 48000) != 0) {
        logMessage("Agora: Failed to set audio frame parameters!");
        return false;
    }


   _connectionObserver = std::make_shared<ConnectionObserver>(this);
   _connection->registerObserver(_connectionObserver.get());
   _connection->registerNetworkObserver(_connectionObserver.get());

    if(_sendOnly==false){
        _connection->getLocalUser()->registerVideoFrameObserver(_yuvFrameObserver.get());
        _connection->getLocalUser()->registerAudioFrameObserver(_pcmFrameObserver.get());
    }

    std::cout<<" connecting to: "<<in_ch_id << "  " <<  _proxyConnectionTimeOut <<std::endl;
    doConnect(in_app_id, in_ch_id, in_user_id);
    if (!checkConnection() && _enableProxy) {
	_connection->disconnect();
        agora::base::IAgoraParameter* agoraParameter = _connection->getAgoraParameter();
        auto ipList=parseIpList();
        if (ipList.size() > 1) {
            auto ipListString=createProxyString(ipList);
            std::cout<< "Set proxy IPs  " << ipList.size() << std::endl;
            agoraParameter->setParameters(ipListString.c_str());
        } else {
            std::cout<< "Enable proxy with default access IPs " << std::endl;
            agoraParameter->setBool("rtc.enable_proxy", true);
        }
       	doConnect(in_app_id, in_ch_id, in_user_id);
    }

    if (checkConnection()==false){

       logMessage("Error connecting to channel");
       std::cout<<"Error connecting to channel\n";
       return false;
    }

    _videoFrameSender=_factory->createVideoEncodedImageSender();
    if (!_videoFrameSender) {
       std::cout<<"Failed to create video frame sender!"<<std::endl;
       return false;
    }

    //if you want to send_dual_h264,the ccMode must be enabled
#if SDK_BUILD_NUM >=200931 //sdk >=3.8
    agora::rtc::SenderOptions option;
#else
     agora::base::SenderOptions option;
#endif
#if SDK_BUILD_NUM >=190534 //sdk >=3.7
     option.ccMode = agora::rtc::TCcMode::CC_ENABLED;
#else
    option.ccMode = agora::base::CC_ENABLED;
#endif
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

    // Publish  video and audio tracks
    startPublishAudio();
    startPublishVideo();

    //initialize encoder and decoder
    if(!initTranscoder()){
        std::cout<<"failed to init transcoder\n";
    }
    
    if(_sendOnly==false){
        if(false){
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
        }else{
            //video
            _receivedVideoFrames=std::make_shared<WorkQueue <Work_ptr> >();
            _yuvFrameObserver->setOnVideoFrameReceivedFn([this](const char* channelId, agora::user_id_t remoteUid, const agora::media::base::VideoFrame* videoFrame){
                receiveDecodedVideoFrame(videoFrame);
            });
        }
        //audio
        _receivedAudioFrames=std::make_shared<WorkQueue <Work_ptr> >();
        _pcmFrameObserver->setOnAudioFrameReceivedFn([this](const uint userId, 
                                                        const uint8_t* buffer,
                                                        const size_t& length,
                                                        const uint64_t& ts){

             receiveAudioFrame(userId, buffer, length,ts);
        });

        //connection observer: handles user join and leave
        _connectionObserver->setOnUserStateChanged([this](const std::string& userId,
                                                      const UserState& newState){
                    
                handleUserStateChange(userId, newState);

        });
    }

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
            addEvent(AGORA_EVENT_ON_USER_STATE_CHANED,userId,USER_STATE_MIC_OFF,0);
        }
        else if(messsage==0 && value==0){
            addEvent(AGORA_EVENT_ON_USER_STATE_CHANED,userId,USER_STATE_MIC_ON,0);
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

        if (_transcodeVideo) {
	        // frm transcode down when 
		//long media_bitrate_bps= (*(stats+10)) ;
		long target_media_bitrate_bps= (*(stats+9)) ;
	 
	        if (target_media_bitrate_bps < 1200000) {
			_requireTranscode=true;
			setTranscoderProps(_transcodeWidthLow, _transcodeHeightLow, *(stats+9), 30);
		} else if (target_media_bitrate_bps < 2000000) {
			_requireTranscode=true;
			setTranscoderProps(_transcodeWidthMedium, _transcodeHeightMedium, *(stats+9), 30);
		} else if (_requireTranscode) {
			_requireTranscode=false;
			_requireKeyframe=true;
		}	
	
	        std::cout<<"BWSTATS target_media_bitrate_bps " << (*(stats+9)) << " media_bitrate_bps " <<  (*(stats+10)) <<  " _requireTranscode " << _requireTranscode << " _requireKeyframe " <<  _requireKeyframe  << " \n";
	}

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
    if(false){
        _inSyncBuffer->setVideoOutFn([this](const uint8_t* buffer,
                                            const size_t& bufferLength,
                                            const bool& isKeyFrame){
    
            if(_videoOutFn!=nullptr){
                _videoOutFn(buffer, bufferLength, _videoOutUserData);
                _videoInFps++;
            }

        });
    }else{
        _inSyncBuffer->setDecodedVideoOutFn([this](const uint8_t* buffer,
                                            const size_t& bufferLength,int width,int height){    
            if(_decodedVideoOutFn!=nullptr){
                _decodedVideoOutFn(buffer, bufferLength,width,height, _videoOutUserData);
                _videoInFps++;
            }
        });
    }

    _inSyncBuffer->setAudioOutFn([this](const uint8_t* buffer,
                                         const size_t& bufferLength){

        
        //TODO: we can print volume for each few seconds to make sure we got audio from the sdk
        //auto volume=calcVol((const int16_t*)buffer, bufferLength/2);
        //if(volume>0){
          // std::cout<<"audio volume: "<<volume<<std::endl;
       // }
        
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

void AgoraIo::receiveDecodedVideoFrame(const agora::media::base::VideoFrame* videoFrame){

    //do not read video if the pipeline is in pause state
    if(_isPaused) return;

    if(_inSyncBuffer!=nullptr && _isRunning){
        _inSyncBuffer->addVideo(videoFrame);
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

void AgoraIo::handleUserStateChange(const std::string& userId, 
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
    else if(newState==USER_LEAVE || newState==USER_CAM_OFF){

         _connection->getLocalUser()->unsubscribeVideo(userId.c_str());
        _activeUsers.remove_if([userId](const std::string& id){ return (userId==id); });
        if(_activeUsers.empty()==false && _currentVideoUser==userId){

            auto newUserId=_activeUsers.front();
            subscribeToVideoUser(newUserId);
        }
    } 
}

 void AgoraIo::subscribeToVideoUser(const std::string& userId){

    if(_sendOnly==false){  
        
#if SDK_BUILD_NUM>=190534 
        agora::rtc::VideoSubscriptionOptions subscriptionOptions;
#else
        agora::rtc::ILocalUser::VideoSubscriptionOptions subscriptionOptions;
#endif
        if(false){
            subscriptionOptions.encodedFrameOnly = true;
        }
        subscriptionOptions.type = agora::rtc::VIDEO_STREAM_HIGH;
        _connection->getLocalUser()->subscribeVideo(userId.c_str(), subscriptionOptions);

        _currentVideoUser=userId;
        std::cout<<"subscribed to video user #"<<_currentVideoUser<<std::endl;

        addEvent(AGORA_EVENT_ON_VIDEO_SUBSCRIBED,userId,0,0);
    }
 }



// void AgoraIo::setOnAudioFrameReceivedFn(const OnNewAudioFrame_fn& fn){
//    _pcmFrameObserver->setOnAudioFrameReceivedFn(fn);
// }

// void AgoraIo::setOnVideoFrameReceivedFn(const OnNewFrame_fn& fn){
//   h264FrameReceiver->setOnVideoFrameReceivedFn(fn);
// }

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

    memcpy(data, work->payload.encoded->buffer, work->payload.encoded->len);

    *is_key_frame=work->payload.encoded->is_key_frame;
    *ts=work->timestamp;

    return work->payload.encoded->len;
}

size_t AgoraIo::getNextAudioFrame(uint8_t* data, size_t max_buffer_size){
   
    _receivedAudioFrames->waitForWork();
    Work_ptr work=_receivedAudioFrames->get();

    memcpy(data, work->payload.encoded->buffer, work->payload.encoded->len);

    return work->payload.encoded->len;
}

void AgoraIo::subscribeAudioUser(const std::string& userId){

    _connection->getLocalUser()->subscribeAudio(userId.c_str());
    std::cout<<"subscribed to audio user "<<userId<<std::endl;
}
void AgoraIo::unsubscribeAudioUser(const std::string& userId){

  _connection->getLocalUser()->unsubscribeAudio(userId.c_str());
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

 // if (is_iframe!=is_key_frame) {
  //	std::cout<<" keyframe detect is_iframe=" << is_iframe <<  " is_key_frame " << is_key_frame << " \n";
  //}

  if (is_iframe && !is_key_frame) {
	  is_key_frame=1;
  }

  auto frameType=agora::rtc::VIDEO_FRAME_TYPE_DELTA_FRAME; 
  if(is_key_frame){
     frameType=agora::rtc::VIDEO_FRAME_TYPE_KEY_FRAME;
  } else {
  }

  agora::rtc::EncodedVideoFrameInfo videoEncodedFrameInfo;
  videoEncodedFrameInfo.rotation = agora::rtc::VIDEO_ORIENTATION_0;
  videoEncodedFrameInfo.codecType = agora::rtc::VIDEO_CODEC_H264;
  videoEncodedFrameInfo.framesPerSecond = 30;
  videoEncodedFrameInfo.frameType = frameType;
  videoEncodedFrameInfo.streamType = agora::rtc::VIDEO_STREAM_HIGH;

  if(_transcodeVideo && (_requireTranscode || (_requireKeyframe && !is_key_frame))){
        //transcoding 
        const uint32_t MAX_FRAME_SIZE=1024*1020;
        std::unique_ptr<uint8_t[]> outBuffer(new uint8_t[MAX_FRAME_SIZE]);
        uint32_t outBufferSize=transcode(buffer, len, outBuffer.get(), is_key_frame);
	if (!_requireTranscode) {
        	std::cout<<"waiting for keyframe, length=  " << len <<  " \n";
	}
	if (outBufferSize>0) {
        	_videoFrameSender->sendEncodedVideoImage(outBuffer.get(),outBufferSize,videoEncodedFrameInfo);
	} else {
        	std::cout<<"encode skipped as no decode \n";
	}
  }
  else{
	// if switching back do it on keyframe 
	if (_requireKeyframe && is_key_frame) {
		_requireKeyframe=false;
	}

	if (!_requireKeyframe) { 
        	//std::cout<<" SENDing encoded, length= " << len << "  \n";
        	_videoFrameSender->sendEncodedVideoImage(buffer,len,videoEncodedFrameInfo);
	} else {
        	std::cout<<" pr encoded skipped as not keyframe  \n";
	}
  }
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

  agora::rtc::EncodedAudioFrameInfo audioFrameInfo;
  audioFrameInfo.numberOfChannels =1; //TODO
  audioFrameInfo.sampleRateHz = 48000; //TODO
  audioFrameInfo.codec = agora::rtc::AUDIO_CODEC_OPUS;

  _audioSender->sendEncodedAudioFrame(buffer,len, audioFrameInfo);

  //auto diff=GetTimeDiff(_lastSendTime, Now());
  // logMessage("sent audo packet. diff time: "+std::to_string(diff));

  //_lastSendTime=Now();

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


   _outSyncBuffer=nullptr;
   _inSyncBuffer=nullptr;
   
   
   //delete context
   _connection=nullptr;

   _service->release();
   _service = nullptr;

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

 void AgoraIo::setDecodedVideoOutFn(agora_decoded_media_out_fn videoOutFn, void* userData){
     _decodedVideoOutFn=videoOutFn;
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
    _connection->getLocalUser()->publishAudio(_customAudioTrack);
    _isPublishingAudio=true;

    std::cout<<"Agoraio: published Audio\n";

 }
void AgoraIo::startPublishVideo(){

    if(_isPublishingVideo==true){
        return;
    }
     _connection->getLocalUser()->publishVideo(_customVideoTrack);
    _isPublishingVideo=true;

    std::cout<<"Agoraio: published video\n";
}

void AgoraIo::stopPublishAudio(){

    if(_isPublishingAudio==false){
        return;
    }
    _connection->getLocalUser()->unpublishAudio(_customAudioTrack);
    _isPublishingAudio=false;

    std::cout<<"Agoraio: unpublished Audio\n";
}
void AgoraIo::stopPublishVideo(){

    if(_isPublishingVideo==false){
        return;
    }

     _connection->getLocalUser()->unpublishVideo(_customVideoTrack);
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
