#include "agora_rtc_api.h"
#include "agoralite.h"
#include "agoratype.h"

#include <string>
#include <cstring>
#include <iostream>
#include <fstream>

#include <unistd.h>
#include <sys/time.h>

#define DEFAULT_CERTIFACTE_FILENAME "certificate.bin"

#define DEFAULT_SEND_VIDEO_FRAME_RATE (30)
#define DEFAULT_BANDWIDTH_ESTIMATE_MIN_BITRATE (100000)
#define DEFAULT_BANDWIDTH_ESTIMATE_MAX_BITRATE (1000000)
#define DEFAULT_BANDWIDTH_ESTIMATE_START_BITRATE (500000)


g_context_t g_ctx;

// get a string from file, return null if file does not exist
char *util_get_string_from_file(const char *path)
{
	FILE *f = fopen(path, "rb");

	if (!f) {
      std::cout<<"Failed to open"<<path<<std::endl;
		return NULL;
	}

	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);

	char *string = (char*)malloc(fsize + 1);
	if (fread(string, 1, fsize, f) != fsize) {
        fclose(f);
        free(string);
		return NULL;
	}
	fclose(f);

	string[fsize] = 0;
	return string;
}

static void __on_join_channel_success(const char *channel, int elapsed)
{
   g_ctx.b_connected_flag = true;
   std::cout<<"->Join the channel: "<<channel<<" successfully, elapsed "<<elapsed<<" ms"<<std::endl;
}
static void __on_connection_lost(const char *channel)
{
   g_ctx.b_connected_flag = false;
   std::cout<<"->Lost connection from the channel: "<<channel<<std::endl;
}

static void __on_rejoin_channel_success(const char *channel, int elapsed_ms)
{
   g_ctx.b_connected_flag = true;
   std::cout<<"->Rejoin the channel: "<<channel<<" successfully, elapsed "<<elapsed_ms<<" ms"<<std::endl;
}

static void __on_user_joined(const char *channel, uint32_t uid, int elapsed_ms)
{
   std::cout<<"->Remote user: "<<uid<<" has joined the channel "<<channel<<std::endl;
}

static void __on_user_offline(const char *channel, uint32_t uid, int reason)
{
	//LOGI("Remote user \"%u\" has left the channel \"%s\", reason %d", uid, channel, reason);
}

static void __on_user_mute_audio(const char *channel, uint32_t uid, int muted)
{
	if(muted>0){
		g_ctx.event_callback_fn(AGORA_EVENT_ON_USER_STATE_CHANED,std::to_string(uid).c_str(),USER_STATE_MIC_OFF,0, g_ctx.event_user_data);
	}
	else{
		g_ctx.event_callback_fn(AGORA_EVENT_ON_USER_STATE_CHANED,std::to_string(uid).c_str(),USER_STATE_MIC_ON,0, g_ctx.event_user_data);
	}
}

static void __on_user_mute_video(const char *channel, uint32_t uid, int muted)
{
	if(muted>0){
		g_ctx.event_callback_fn(AGORA_EVENT_ON_USER_STATE_CHANED,std::to_string(uid).c_str(),USER_STATE_CAM_OFF,0, g_ctx.event_user_data);
	}
	else{
		g_ctx.event_callback_fn(AGORA_EVENT_ON_USER_STATE_CHANED,std::to_string(uid).c_str(),USER_STATE_CAM_ON,0, g_ctx.event_user_data);
	}
}

static void __on_error(const char *channel, int code, const char *msg)
{
	if (code == ERR_SEND_VIDEO_OVER_BANDWIDTH_LIMIT) {
		std::cout<<"Not enough uplink bandwdith. Error msg:" << msg<<std::endl;
		return;
	}

	if (code == ERR_INVALID_APP_ID) {
		std::cout<<"Invalid App ID. Please double check. Error msg: "<<msg<<std::endl;
	} else if (code == ERR_INVALID_CHANNEL_NAME) {
		std::cout<<"Invalid channel name: "<<channel<<". Please double check. Error msg \"%s\""<< msg;
	} else if (code == ERR_INVALID_TOKEN || code == ERR_TOKEN_EXPIRED) {
		std::cout<<"Invalid token. Please double check. Error msg: "<< msg<<std::endl;
	} else if (code == ERR_DYNAMIC_TOKEN_BUT_USE_STATIC_KEY) {
		std::cout<<"Dynamic token is enabled but is not provided. Error msg: "<< msg<<std::endl;;
	} else {
		std::cout<<"Error" <<code<< " is captured. Error msg: "<<msg<<std::endl;
	}

	g_ctx.b_stop_flag = true;
}

static void __on_audio_data(const char *channel, const uint32_t uid, uint16_t sent_ts,
							audio_data_type_e data_type, const void *data, size_t len)
{
	if(g_ctx.audio_callback_fn!=NULL && 
	   g_ctx.b_connected_flag==true){
		g_ctx.audio_callback_fn(uid, (const u_int8_t*)data, len, 0, g_ctx.audio_user_data);
	}
}

static void __on_mixed_audio_data(const char *channel, audio_data_type_e data_type,
								  const void *data, size_t len)
{
	if(g_ctx.audio_callback_fn!=NULL && 
	   g_ctx.b_connected_flag==true){
	  g_ctx.audio_callback_fn(0, (const u_int8_t*)data, len,0, g_ctx.audio_user_data);
	}
}

static void __on_video_data(const char *channel, const uint32_t uid, uint16_t sent_ts,
							video_data_type_e data_type, uint8_t stream_id, int is_key_frame,
							const void *data, size_t len)
{
	if(g_ctx.video_callback_fn!=NULL && 
	   g_ctx.b_connected_flag==true){
	   g_ctx.video_callback_fn(uid, (const u_int8_t*)data, len, is_key_frame, g_ctx.video_user_data);
	}
}

static void __on_target_bitrate_changed(const char *channel, uint32_t target_bps)
{
   std::cout<<"->bitrate changed "<<std::endl;
}

static void __on_key_frame_gen_req(const char *channel, uint32_t uid, uint8_t stream_id)
{
	g_ctx.event_callback_fn(AGORA_EVENT_ON_IFRAME,std::to_string(uid).c_str(),0,0, g_ctx.event_user_data);

   //std::cout<<"->n_key_frame_gen_req "<<std::endl;
}

static void app_init_event_handler(agora_rtc_event_handler_t *event_handler)
{
	event_handler->on_join_channel_success = __on_join_channel_success;
	event_handler->on_connection_lost = __on_connection_lost;
	event_handler->on_rejoin_channel_success = __on_rejoin_channel_success;
	event_handler->on_user_joined = __on_user_joined;
	event_handler->on_user_offline = __on_user_offline;
	event_handler->on_user_mute_audio = __on_user_mute_audio;
	event_handler->on_user_mute_video = __on_user_mute_video;
	event_handler->on_target_bitrate_changed = __on_target_bitrate_changed;
	event_handler->on_key_frame_gen_req = __on_key_frame_gen_req;
	event_handler->on_error = __on_error;

    if(g_ctx.is_send_only==false){
		event_handler->on_mixed_audio_data = __on_mixed_audio_data;
		event_handler->on_video_data = __on_video_data;
	}
}

void util_sleep_ms(int64_t ms)
{
	usleep(ms * 1000);
}

g_context_t* agora_lite_init(char* app_id, char* ch_id, char* user_id){
	 
	int rval;

	g_ctx.video_callback_fn=NULL;
	g_ctx.audio_callback_fn=NULL;


   //API: verify license
	char *str_certificate = util_get_string_from_file(DEFAULT_CERTIFACTE_FILENAME);
	if (str_certificate) {
		rval = agora_rtc_license_verify(str_certificate, strlen(str_certificate), NULL, 0);
		if (rval < 0) {
         std::cout<<"Failed to verify license, reason:"<<agora_rtc_err_2_str(rval)<<std::endl;
			return nullptr;
		}
		std::cout<<"Verify license successfully"<<std::endl;
	}


   //API: init agora rtc sdk
	int appid_len = strlen(app_id);

    agora_rtc_event_handler_t event_handler = { 0 };
	app_init_event_handler(&event_handler);

    rtc_service_option_t service_opt = { 0 };
	service_opt.area_code = AREA_CODE_GLOB;
	service_opt.log_cfg.log_path = "io.agora.rtc_sdk";

	rval = agora_rtc_init(app_id, &event_handler, &service_opt);
	if (rval < 0) {
      std::cout<<"Failed to initialize Agora sdk: "<<agora_rtc_err_2_str(rval)<<std::endl;
		return nullptr;
	}

	std::cout<<"Successfully inited agora,  "<<rval<<std::endl;

    agora_rtc_set_bwe_param(DEFAULT_BANDWIDTH_ESTIMATE_MIN_BITRATE,
							DEFAULT_BANDWIDTH_ESTIMATE_MAX_BITRATE,
							DEFAULT_BANDWIDTH_ESTIMATE_START_BITRATE);

    rtc_channel_options_t channel_options;
	memset(&channel_options, 0, sizeof(channel_options));
	channel_options.auto_subscribe_audio = true;
	channel_options.auto_subscribe_video = true;


	rval = agora_rtc_join_channel(ch_id, user_id, NULL, 0,
								  &channel_options);
	if (rval < 0) {

      std::cout<<"Failed to join channel "<<ch_id<<" reason "<< agora_rtc_err_2_str(rval)<<std::endl;
	  return nullptr;
	}

	std::cout<<"Successfully joined the channel "<<ch_id<<", "<<rval<<std::endl;

    //wait a bit untill the channels is joined
	int iter=0;
	while(g_ctx.b_connected_flag==false &&
	      ++iter<1000){
		util_sleep_ms(200);
	}

	free(str_certificate);

    return &g_ctx;
}

int agora_lite_send_video(const unsigned char* buffer,  
						  unsigned long len, 
						  int is_key_frame,
						  long timestamp)
{
	uint8_t stream_id = 0;

	// API: send vido data
	video_frame_info_t info;
	info.type = is_key_frame ? VIDEO_FRAME_KEY : VIDEO_FRAME_DELTA;
	info.frames_per_sec = (video_frame_rate_e)(30);
	info.data_type = VIDEO_DATA_TYPE_H264;

	int rval = agora_rtc_send_video_data(g_ctx.channel_id.c_str(), stream_id, buffer, len, &info);
	if (rval < 0) {
		std::cout<<"Failed to send video data, reason: "<< agora_rtc_err_2_str(rval)<<std::endl;
		return -1;
	}

	return 0;
}

int agora_lite_send_audio(const unsigned char* buffer,  
					  unsigned long len)
{
	// API: send audio data
	audio_frame_info_t info; //= { 0 };
	info.data_type = AUDIO_DATA_TYPE_OPUS;
	int rval = agora_rtc_send_audio_data(g_ctx.channel_id.c_str(), buffer, len, &info);
	if (rval < 0) {
		std::cout<<"Failed to send audio data, reason: %s"<< agora_rtc_err_2_str(rval)<<std::endl;
		return -1;
	}

	return 0;
}


int agora_lite_disconnect(){

    g_ctx.b_connected_flag=false;
	agora_rtc_leave_channel(g_ctx.channel_id.c_str());

	agora_rtc_fini();

	return 0;
}

