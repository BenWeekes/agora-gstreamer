#ifndef _AGORA_C_H_
#define _AGORA_C_H_
#include <stdbool.h>
#ifdef __cplusplus
 #define EXTERNC extern "C"
 #else
 #define EXTERNC
 #endif

 #include "agoraconfig.h"


 typedef  struct agora_context_t agora_context_t;
 typedef  void (*agora_log_func_t)(void*, const char*);

 typedef  void (*agora_media_out_fn)(const u_int8_t* buffer,
                                     u_int64_t len,
									 void* user_data);

 typedef struct agora_receive_context_t agora_receive_context_t;

 typedef struct AgoraIoContext_t  AgoraIoContext_t;


 EXTERNC AgoraIoContext_t*  agoraio_init(agora_config_t* config);

EXTERNC void agoraio_disconnect(AgoraIoContext_t** ctx);


 EXTERNC int  agoraio_send_video(AgoraIoContext_t* ctx,  
                                const unsigned char* buffer,  
							    unsigned long len, 
								int is_key_frame,
							    long timestamp);

 EXTERNC void  agoraio_set_paused(AgoraIoContext_t* ctx, int flag);
								

EXTERNC int  agoraio_send_audio(AgoraIoContext_t* ctx,  
                               const unsigned char* buffer,  
							   unsigned long len,
							   long timestamp);

 EXTERNC int  agoraio_send_audio_with_duration(AgoraIoContext_t* ctx,  
                               const unsigned char* buffer,  
							   unsigned long len,
							   long timestamp,
							   long  duration);

 EXTERNC void agora_set_log_function(agora_context_t* ctx, agora_log_func_t f, void* log_ctx);

 EXTERNC void agora_log_message(agora_context_t* ctx, const char* message);

 EXTERNC void agora_dump_audio_to_file(agora_context_t* ctx, unsigned char* data, short sampleCount);


 EXTERNC void logText(const char* message);


EXTERNC  void agoraio_set_event_handler(AgoraIoContext_t* ctx, event_fn fn, void* userData);

EXTERNC  void agoraio_set_video_out_handler(AgoraIoContext_t* ctx, agora_media_out_fn fn, void* userData);

EXTERNC  void agoraio_set_audio_out_handler(AgoraIoContext_t* ctx, agora_media_out_fn fn, void* userData);

EXTERNC void agoraio_set_sendonly_flag(AgoraIoContext_t* ctx, int flag);

#undef EXTERNC


#endif
