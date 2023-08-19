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
   agora::media::base::VideoFrame frame;
   std::vector<uint8_t> buffer_;
   bool cached = false;
   void cache(){
      if(!cached){
        int ySize = frame.yStride * frame.height;
        int uSize = frame.uStride * frame.height / 2;
        int vSize = frame.vStride * frame.height / 2;        
        buffer_.resize(ySize + uSize + vSize);
        std::memcpy(buffer_.data(), frame.yBuffer, ySize);
        std::memcpy(buffer_.data() + ySize, frame.uBuffer, uSize);
        std::memcpy(buffer_.data() + ySize + uSize, frame.vBuffer, vSize);
        cached = true;
      }
   }
   public:   
   const std::vector<uint8_t>& buffer() {
        if(!cached){
          cache();
        }
        return buffer_;
    }
   static FramePayloadDecoded* shallow(const agora::media::base::VideoFrame* videoFrame) {
        auto payload =  new FramePayloadDecoded();
        payload->frame = *videoFrame;
        return payload;
   }
   static FramePayloadDecoded* deep(const agora::media::base::VideoFrame* videoFrame) {
        auto payload =  new FramePayloadDecoded();
        payload->frame = *videoFrame;
        payload->cache();
        return payload;
   }
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
   Work(const agora::media::base::VideoFrame* videoFrame){
       is_finished=0;
       frame_type = decoded;
       payload.decoded = FramePayloadDecoded::shallow(videoFrame);
   }
   ~Work(){
      if(frame_type == encoded && payload.encoded != nullptr){
         delete payload.encoded;
         payload.encoded = nullptr;
      }
      if(frame_type == decoded && payload.decoded != nullptr){
         delete payload.decoded;
         payload.decoded = nullptr;
      }
   }
   bool                  is_finished;
   uint64_t               timestamp;
};
#endif
