#ifndef __AGORA_LITE_H__
#define __AGORA_LITE_H__

#include <string>

typedef  void (*agora_media_fn)(u_int64_t user_id,
	                                 const u_int8_t* buffer,
                                     u_int64_t len,
									 bool   is_key_frame,
									 void* user_data);

typedef  void (*agora_event_fn)(const int& eventType, 
                  				const char* userName,
                  				const long& param1, 
                  				const long& param2,
								void* user_data);


//a global context to communicate between the app and agora sdk
struct g_context_t{
   bool   		b_connected_flag;
   bool   		b_stop_flag;
   std::string  channel_id;

   bool         is_send_only;

   agora_media_fn   video_callback_fn;
   void*            video_user_data;

   agora_media_fn   audio_callback_fn;
   void* 		    audio_user_data;

   agora_event_fn   event_callback_fn;
   void* 		    event_user_data;
};
g_context_t* agora_lite_init(char* app_id, char* ch_id, char* user_id);

int agora_lite_send_video(const unsigned char* buffer,  
						        unsigned long len, 
						        int is_key_frame,
						        long timestamp);

int agora_lite_send_audio(const unsigned char* buffer,  
					           unsigned long len);

int agora_lite_disconnect();

#endif