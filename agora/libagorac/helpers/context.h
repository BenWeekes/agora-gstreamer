#ifndef _CONTEXT_H_
#define _CONTEXT_H_

#include "../agoratype.h"
#include "../observer/connectionobserver.h"

#if SDK_BUILD_NUM>=190534 
#include "IAgoraService.h"
#include "NGIAgoraLocalUser.h"
#include "NGIAgoraAudioTrack.h"

#include "NGIAgoraMediaNodeFactory.h"
#include "NGIAgoraMediaNode.h"
#include "NGIAgoraVideoTrack.h"
#include "NGIAgoraRtcConnection.h"
#endif

using AgoraVideoSender_ptr=agora::agora_refptr<agora::rtc::IVideoEncodedImageSender>;
using AgoraAudioSender_ptr=agora::agora_refptr<agora::rtc::IAudioEncodedFrameSender>;
using AgoraVideoFrameType=agora::rtc::VIDEO_FRAME_TYPE;
using ConnectionConfig=agora::rtc::RtcConnectionConfiguration;

//a context that group all require info about agora
struct agora_context_t{

  agora::base::IAgoraService*                      service;
  agora::agora_refptr<agora::rtc::IRtcConnection>  connection;

  int                                              isConnected;

  ConnectionConfig                                 config;

  AgoraVideoSender_ptr                             videoSender;
  AgoraAudioSender_ptr                             audioSender;

  std::shared_ptr<std::thread>                    videoThreadHigh;
  std::shared_ptr<std::thread>                    videoThreadLow;

  std::shared_ptr<std::thread>                    audioThread;

  agora::agora_refptr<agora::rtc::ILocalAudioTrack> audioTrack;
  agora::agora_refptr<agora::rtc::ILocalVideoTrack> videoTrack;

  bool                                            isRunning;

  AgoraDecoder_ptr                                videoDecoder;
  AgoraEncoder_ptr                                videoEncoder;


  bool                                            encodeNextFrame;
  bool                                            enable_dual;
  agora_log_func_t                                log_func;
  void                                            *log_ctx;

  //std::shared_ptr<MyUserObserver>                 localUserObserver;

  uint8_t                                         fps;
  float                                           predictedFps;

  TimePoint                                       lastFpsPrintTime; 
  TimePoint                                       lastBufferingTime;

  uint16_t                                        jb_size; 

  LocalConfig_ptr                                 callConfig;

  uint64_t                                        reBufferingCount;    

  long                                            lastFrameTimestamp; 
  long                                            timestampPerSecond; 

  TimePoint                                       lastHighFrameSendTime;
  TimePoint                                       lastLowFrameSendTime;

   long                                            lastVideoTimestamp;
   long                                            lastAudioTimestamp;

   int                                             lastVideoSampingInterval;

   WorkQueue_ptr                                   videoJB;
   WorkQueue_ptr                                   audioJB;  

   WorkQueue_ptr                                   videoQueueLow;

   bool                                            isJbBuffering;   

   uint16_t                                        dfps; 

   uint8_t                                         highVideoFrameCount;
   uint8_t                                         lowVideoFrameCount;  

   std::string                                     audioDumpFileName;   

   ConnectionObserver_ptr                         _connectionObserver;      
};

class AgoraReceiverUser;
struct agora_receive_context_t{

   std::shared_ptr<AgoraReceiverUser> _receiver;
};

class AgoraIo;
struct AgoraIoContext_t{

   std::shared_ptr<AgoraIo>  agoraIo;
};


class FramePayloadEncoded {
public:
   FramePayloadEncoded(const unsigned char* b, const unsigned long& l, bool is_key):
	buffer(nullptr){
       if(b==nullptr){
	       return;
       }
       buffer=new unsigned char[l];
       memcpy(buffer, b, l);
       len=l;    
       is_key_frame=is_key;
   }

   ~FramePayloadEncoded(){
     if(buffer==nullptr) return;
     delete [] buffer;
   }
   unsigned char*        buffer;
   unsigned long         len;
   bool                  is_key_frame;
};
class FramePayloadDecoded{
   public:
};
union FramePayload {
   public:
      FramePayloadEncoded* encoded;
      FramePayloadDecoded* decoded;
};
enum FrameType{
  encoded,decoded
};
class Work{

public:
   FrameType frame_type;
   FramePayload payload;
   Work(const unsigned char* b, const unsigned long& l, bool is_key){
       is_finished=0;
       frame_type = encoded;
       payload.encoded = new FramePayloadEncoded(b, l, is_key);
   }

   ~Work(){
      if(frame_type == encoded){
         delete payload.encoded;
      }
   }
   bool                  is_finished;
   uint64_t               timestamp;
};
#endif
