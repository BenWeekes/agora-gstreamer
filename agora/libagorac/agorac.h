#ifndef _AGORA_C_H_
#define _AGORA_C_H_
#include <stdbool.h>
#ifdef __cplusplus
 #define EXTERNC extern "C"
 #else
 #define EXTERNC
 #endif

 typedef  struct agora_context_t agora_context_t;
 typedef  void (*agora_log_func_t)(void*, const char*);

 typedef struct agora_receive_context_t agora_receive_context_t;

 typedef struct AgoraIoContext_t  AgoraIoContext_t;

 EXTERNC agora_context_t*  agora_init(char* app_id, char* ch_id, char* user_id,
                                    bool is_audiouser,
                                    bool enc_enable,
		                            short enable_dual, unsigned int  dual_vbr, 
				                    unsigned short  dual_width,  unsigned short  dual_height,
									unsigned short min_video_jb, unsigned short dfps);

 EXTERNC AgoraIoContext_t*  agoraio_init2(char* app_id, char* ch_id, char* user_id,
                                        bool is_audiouser,
                                        bool enc_enable,
		                                short enable_dual,
										unsigned int  dual_vbr, 
				                        unsigned short  dual_width, 
										unsigned short  dual_height,
									    unsigned short min_video_jb,
										unsigned short dfps,
										bool verbose);

 EXTERNC agora_receive_context_t* agora_receive_init(char* app_id,
                                                     char* ch_id, 
													 char* user_id,
													 int receiveAudio,
													 int receiveVideo,
													 int verbose,
													 char* filePath);

 EXTERNC int  agora_send_video(agora_context_t* ctx,  
                               const unsigned char* buffer,  
							    unsigned long len, 
								int is_key_frame,
							    long timestamp);

 EXTERNC int  agoraio_send_video(AgoraIoContext_t* ctx,  
                                const unsigned char* buffer,  
							    unsigned long len, 
								int is_key_frame,
							    long timestamp);
								
 EXTERNC int  agora_send_audio(agora_context_t* ctx,  
                               const unsigned char* buffer,  
							   unsigned long len,
							   long timestamp);

EXTERNC int  agoraio_send_audio(AgoraIoContext_t* ctx,  
                               const unsigned char* buffer,  
							   unsigned long len,
							   long timestamp);

 EXTERNC void agora_disconnect(agora_context_t** ctx);

 EXTERNC void agora_set_log_function(agora_context_t* ctx, agora_log_func_t f, void* log_ctx);

 EXTERNC void agora_log_message(agora_context_t* ctx, const char* message);

 EXTERNC void agora_dump_audio_to_file(agora_context_t* ctx, unsigned char* data, short sampleCount);

 EXTERNC size_t get_next_video_frame(agora_receive_context_t* context, 
                                     unsigned char* data, size_t max_buffer_size,
									 int* is_key_frame);

EXTERNC size_t agoraio_read_video(AgoraIoContext_t* ctx, 
                                   unsigned char* data, size_t max_buffer_size,
								   int* is_key_frame);

EXTERNC size_t agoraio_read_audio(AgoraIoContext_t* ctx, 
                                  unsigned char* data, size_t max_buffer_size);

 EXTERNC size_t get_next_audio_frame(agora_receive_context_t* context, 
                                     unsigned char* data, size_t max_buffer_size);
 #undef EXTERNC


#endif
