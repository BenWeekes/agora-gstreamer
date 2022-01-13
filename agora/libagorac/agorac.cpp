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

int agoraio_send_audio(AgoraIoContext_t* ctx,
                     const unsigned char * buffer, 
                     unsigned long len,
                     long timestamp){

    if(ctx->agoraIo!=nullptr){
        ctx->agoraIo->sendAudio(buffer, len, timestamp);
     }

    return 0;
}

int  agoraio_send_audio_with_duration(AgoraIoContext_t* ctx,  
                                       const unsigned char* buffer,  
							                  unsigned long len,
							                  long timestamp,
							                  long  duration){
   if(ctx->agoraIo!=nullptr){
        ctx->agoraIo->sendAudio(buffer, len, timestamp, duration);
     }

   return 0;
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

AgoraIoContext_t*  agoraio_init(char* app_id, char* ch_id, char* user_id,
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
										          void* userData,
                                        int in_audio_delay,
										          int in_video_delay,
										          int out_audio_delay,
										          int out_video_delay,
                                        int sendOnly){

    AgoraIoContext_t* ctx=new AgoraIoContext_t;
    if(ctx==nullptr){
        return NULL;
    }

    ctx->agoraIo=std::make_shared<AgoraIo>(verbose,
                                           fn,
                                           userData, 
                                           in_audio_delay,
                                           in_video_delay,
                                           out_audio_delay,
                                           out_video_delay,
                                           sendOnly);

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

void agoraio_set_video_out_handler(AgoraIoContext_t* ctx, agora_media_out_fn fn, void* userData){
   
   if(ctx==nullptr)  return;

   ctx->agoraIo->setVideoOutFn(fn, userData);
}

void agoraio_set_audio_out_handler(AgoraIoContext_t* ctx, agora_media_out_fn fn, void* userData){

   if(ctx==nullptr)  return;

   ctx->agoraIo->setAudioOutFn(fn, userData);
}

void agoraio_set_sendonly_flag(AgoraIoContext_t* ctx, int flag){

  if(ctx==nullptr)  return;

   ctx->agoraIo->setSendOnly(flag);
}