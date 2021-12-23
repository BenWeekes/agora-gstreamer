#include "syncbuffer.h"
#include "helpers/context.h"
#include "helpers/utilities.h"
#include <iostream>

#include "helpers/agoralog.h"

const size_t MAX_BUFFER_SIZE=200;

static int g_id=0;
SyncBuffer::SyncBuffer(const uint16_t& videoDelayOffset,
                           const uint16_t& audioDelayOffset,
                           const bool& syncAudioVideo):
_videoBuffer(nullptr),
_audioBuffer(nullptr),
_videoSendThread(nullptr),
_audioSendThread(nullptr),
_isRunning(false),
_videoOutFn(nullptr),
_audioOutFn(nullptr),
_maxBufferSize(300),  //in ms
_isJbBuffering(false),
_videoDelayOffset(videoDelayOffset),
_audioDelayOffset(audioDelayOffset),
_syncAudioVideo(syncAudioVideo)
{

  _videoBuffer=std::make_shared<WorkQueue <Work_ptr> >();
  _audioBuffer=std::make_shared<WorkQueue <Work_ptr> >();

  _objId=g_id++;

}

void SyncBuffer::addVideo(const uint8_t* buffer,
                            const size_t& length,
                            const int& isKeyFrame,
                            const uint64_t& ts){

    if(_videoBuffer->size()>MAX_BUFFER_SIZE){
        std::cout<<"JB#"<<_objId<<": warning: sync buffer (video) exceeded max buffer: "<<_videoBuffer->size()<<std::endl;
    }

    auto frame=std::make_shared<Work>(buffer, length,isKeyFrame);
    frame->timestamp=ts;
    _videoBuffer->add(frame);

}

void SyncBuffer::addAudio(const uint8_t* buffer,
                            const size_t& length,
                            const uint64_t& ts){

    if(_audioBuffer->size()>MAX_BUFFER_SIZE){
        std::cout<<"JB#"<<_objId<<": warning: sync buffer (audio) exceeded max buffer: "<<_audioBuffer->size()<<std::endl;
    }
    
    auto frame=std::make_shared<Work>(buffer, length, false);
    frame->timestamp=ts;
    _audioBuffer->add(frame);

}

void SyncBuffer::videoThread(){

   uint8_t currentFramePerSecond=0;

   long lastTimestamp=0;
   TimePoint  lastSendTime=Now();

   while(_isRunning==true){

     checkAndFillInVideoJb(lastSendTime);

     //wait untill work is available
     _videoBuffer->waitForWork();	  
     Work_ptr work=_videoBuffer->get();

     if(work==nullptr) continue;
     if(work->is_finished==1) break;

     //for the first send
     if(lastTimestamp==0){
        lastTimestamp=work->timestamp;
     }

     TimePoint  nextSample=getNextSamplingPoint(_videoBuffer,work->timestamp,lastTimestamp);

     if(_videoOutFn!=nullptr){
         _videoOutFn(work->buffer, work->len, (bool)(work->is_key_frame));         
     }

     lastTimestamp=work->timestamp;

     lastSendTime=Now();

    /*if(_objId==1)
     std::cout<<this<<"VIDEO ts: "<<work->timestamp<<std::endl;*/

     //sleep until our next frame time
     std::this_thread::sleep_until(nextSample);
  }

}
void SyncBuffer::audioThread(){

    const int waitTimeForBufferToBeFull=10;
    unsigned long lastTimestamp=0;

    while(_isRunning==true){

     //wait untill work is available
     _audioBuffer->waitForWork();
     Work_ptr work=_audioBuffer->get();
     if(work==NULL) continue;

     if(work->is_finished){
        return;
     }

     //for the first send
     if(lastTimestamp==0){
        lastTimestamp=work->timestamp;
     }

    TimePoint  nextSample=getNextSamplingPoint(_audioBuffer,work->timestamp,lastTimestamp);

     //when video JB is buffering, audio should buffer too
     while(_isJbBuffering && _isRunning){
        std::this_thread::sleep_for(std::chrono::milliseconds(waitTimeForBufferToBeFull));
     }

     if(_audioOutFn!=nullptr){
         _audioOutFn(work->buffer, work->len);
     }
    /*if(_objId==1)
     std::cout<<this<<"audio ts: "<<work->timestamp<<std::endl;*/

     std::this_thread::sleep_until(nextSample);
   }
}

void SyncBuffer::start(){
    _isRunning=true;

    _videoSendThread=std::make_shared<std::thread>(&SyncBuffer::videoThread,this);
    _audioSendThread=std::make_shared<std::thread>(&SyncBuffer::audioThread,this);

    _videoSendThread->detach();
    _audioSendThread->detach();
}
void SyncBuffer::stop(){
     _isRunning=false;

     Work_ptr work=std::make_shared<Work>(nullptr,0, false);
     work->is_finished=true;

     _videoBuffer->add(work);
     _audioBuffer->add(work);
}

void SyncBuffer::setVideoOutFn(const videoOutFn_t& fn){
    _videoOutFn=fn;
}
void SyncBuffer::setAudioOutFn(const audioOutFn_t& fn){
    _audioOutFn=fn;
}

TimePoint SyncBuffer::getNextSamplingPoint(const WorkQueue_ptr& q, 
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
     if(bufferSizeMs>_maxBufferSize){
        threadsleepTime=(long)(threadsleepTime*0.95);
     }

     const int MAX_SLEEP=300;
     if(threadsleepTime>MAX_SLEEP){
         std::cout<<"currentTimestamp: "<<currentTimestamp<<std::endl;
         std::cout<<"lastTimestamp: "<<lastTimestamp<<std::endl;
         std::cout<<"threadsleepTime: "<<threadsleepTime<<std::endl;
         threadsleepTime=30;
     }


    TimePoint  nextSample = Now()+std::chrono::milliseconds(threadsleepTime);

     return nextSample;
}

void SyncBuffer::checkAndFillInVideoJb(const TimePoint& lastSendTime){

 bool  waitForBufferToBeFull=false;
 int fps=30;

  //check if no frames arrive for 3 seconds. If so, fill the buffer with frames
  auto diff=GetTimeDiff(lastSendTime, Now());
  if(diff>3*fps && 
     _videoBuffer->isEmpty()){

	    waitForBufferToBeFull=true;
  }

  //should wait for the buffer to have min size
  if(waitForBufferToBeFull==true ){
      WaitForBuffering();
  }
}

void SyncBuffer::WaitForBuffering(){

  int fps=30;
  const uint8_t MAX_ITERATIONS=200;
  const int     waitTimeForBufferToBeFull=10; //ms
  auto          timePerFrame=(1000/(float)fps);

  bool buffered=false;

  //wait untill the buffer has min buffer size again
  int waitForBufferingCount=0;
  while(_isRunning==true &&
        waitForBufferingCount<MAX_ITERATIONS && 
        (_videoBuffer->size()*timePerFrame)<_maxBufferSize){

      _isJbBuffering=true;
      std::this_thread::sleep_for(std::chrono::milliseconds(waitTimeForBufferToBeFull));

      waitForBufferingCount++;
      buffered=true;
  }

  if(buffered){
      std::cout <<this<<": buffering ..."<<std::endl;
  }

  _isJbBuffering=false;
}