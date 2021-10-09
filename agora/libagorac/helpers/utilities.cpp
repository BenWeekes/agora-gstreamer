#include "utilities.h"
#include "NGIAgoraRtcConnection.h"
#include "IAgoraService.h"
#include "AgoraBase.h"

#include "agoradecoder.h"
#include "agoraencoder.h"
#include "agoralog.h"
#include "localconfig.h"

#include "agoraconstants.h"

//#include "../userobserver.h"
#include "context.h"

TimePoint Now(){
   return std::chrono::steady_clock::now();
}

long GetTimeDiff(const TimePoint& start, const TimePoint& end){
  return std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
}

std::string GetAddressAsString(agora_context_t* ctx){
   return std::to_string((long)(ctx))+": ";
}

TimePoint GetNextSamplingPoint(const std::string& label,
                               agora_context_t* ctx,
                               const WorkQueue_ptr& q, 
                               const long& currentTimestamp,
                               const long& lastTimestamp){

    //calculate the next ts to wait
     auto currentTs=currentTimestamp;
     auto nextframe=q->top();
     auto nextTs=(nextframe==nullptr)? currentTs: nextframe->timestamp;
     currentTs=(currentTs==nextTs)? lastTimestamp:currentTs;

     auto threadsleepTime=nextTs-currentTs;

     //speedup if we exceed current buffer size
     auto bufferSizeMs=q->size()*threadsleepTime;
     if(bufferSizeMs>ctx->jb_size){
        threadsleepTime=(long)(threadsleepTime*0.95);
     }

     TimePoint  nextSample = Now()+std::chrono::milliseconds(threadsleepTime);
     //logMessage("Sleep time for  "+label+std::to_string(ThreadsleepTime));

      ctx->lastVideoSampingInterval=threadsleepTime;

     return nextSample;
}

//TODO: this function is not used currently 
TimePoint GetNextSamplingPoint(agora_context_t* ctx,
                                const float& speed,
                                const WorkQueue_ptr& q){
  
  //error in fps prediction (in ms)
  auto   timePerFrame=std::ceil(1000/speed);
  auto   samplingFrequency=(long)(timePerFrame);

  //if we have more video frames in the queue, decrease sending time a bit
  if((q->size()-1)*timePerFrame>ctx->jb_size){

      samplingFrequency = (long)(samplingFrequency*9/10.0);

      if(ctx->callConfig->useSpeedupLog()){
           logMessage(GetAddressAsString(ctx)+"speeding up frame consumption by 10%: " +
                                   std::to_string((int)(q->size()*timePerFrame))+
                                   "/"+
                                   std::to_string(ctx->jb_size)
                 );
      }
  }

  TimePoint  nextSample = Now()+std::chrono::milliseconds(samplingFrequency);
 
  
  return nextSample;
}

void WaitForBuffering(agora_context_t* ctx){

  const uint8_t MAX_ITERATIONS=200;
  const int     waitTimeForBufferToBeFull=10; //ms
  auto          timePerFrame=(1000/(float)ctx->predictedFps);

  bool buffered=false;

  //wait untill the buffer has min buffer size again
  int waitForBufferingCount=0;
  while(ctx->isRunning==true &&
        waitForBufferingCount<MAX_ITERATIONS && 
        (ctx->videoJB->size()*timePerFrame)<ctx->jb_size){

      ctx->isJbBuffering=true;
      std::this_thread::sleep_for(std::chrono::milliseconds(waitTimeForBufferToBeFull));
	    logMessage(GetAddressAsString(ctx)+"Video-JB: buffering ("+
                                   std::to_string((int)(ctx->videoJB->size()*timePerFrame))+
                                   "/"+
                                   std::to_string(ctx->jb_size)+
                                  ") ...");

      waitForBufferingCount++;
      buffered=true;
  }
 
  if(buffered){
    logMessage(GetAddressAsString(ctx)+"Video-JB: ----- end buffering -----");
  }

  ctx->isJbBuffering=false;
}

void ResizeBuffer(agora_context_t* ctx){

   const uint8_t MAX_REBUFFERING_COUNT=1;  

   //double JB size if the buffer  keeps running out to frquently
   auto diffBufferingTime=GetTimeDiff(ctx->lastBufferingTime, Now());
   if(ctx->reBufferingCount>=MAX_REBUFFERING_COUNT &&   //just ignore the first buffering, which might not be real
        diffBufferingTime<ctx->callConfig->getTimeToIncreaseJbSize()*1000){

        ctx->jb_size = ctx->jb_size*2;

        logMessage(GetAddressAsString(ctx)+"Video-JB: JB size increased to "+
             std::to_string(ctx->jb_size)+", because it runs out too frequently");
     }

     ctx->lastBufferingTime=Now();
     ctx->reBufferingCount++;
}

bool isNumber(const std::string& userIdString)
{
    return !userIdString.empty() && std::find_if(userIdString.begin(), userIdString.end(), [](unsigned char ch)
		    { return !std::isdigit(ch); }) == userIdString.end();
}

int getVideoSyncBytesPos(const uint8_t* buffer){
	
  if(buffer[0] !=0) return 0;
  if(buffer[1] !=0) return 0;
  if(buffer[2] ==1) return 3;

  if(buffer[2] !=0) return 0;

  if(buffer[3] ==1) return 4;
	
  return 0;
}


