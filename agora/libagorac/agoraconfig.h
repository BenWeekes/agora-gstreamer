#ifndef _AGORA_CONFIG_H_
#define _AGORA_CONFIG_H_

typedef void (*event_fn)(void* userData, 
                         int type, 
                         const char* userName,
                         long param1,
                         long param2,
						 long* states);
typedef struct{
    char*           app_id;
    char*           ch_id;
    char*           user_id;
    bool            is_audiouser;
    bool            enc_enable;
	short           enable_dual;
	unsigned int    dual_vbr; 
	unsigned short  dual_width; 
	unsigned short  dual_height;
	unsigned short  min_video_jb;
	unsigned short  dfps;
	bool            verbose;
	event_fn        fn;
	void*           userData;
	int             in_audio_delay;
	int             in_video_delay;
	int             out_audio_delay;
	int             out_video_delay;
	int             sendOnly;
	int             enableProxy;
	int             proxy_timeout;
	char*           proxy_ips;
	bool            transcode;
}agora_config_t;

#endif
