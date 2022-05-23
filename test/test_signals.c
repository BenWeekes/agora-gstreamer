#include <gst/gst.h>
#include <stdio.h>

#define MAX_BUFFER 1024*5

void on_agora_iframe_request_fn(GstElement* object,gpointer user_data){

    g_print("->Signal Test: iframe requested!\n");
}

void on_agora_on_connecting_fn(GstElement* object,gpointer user_data){

    g_print("->Signal Test: on connecting!\n");
}

void on_agora_on_connected_fn(GstElement* object,
                              gchararray userName,
							  gint reason,
                              gpointer user_data){

    g_print("->Signal Test: on connected: userid=%s, reason=%d\n", userName, reason);
}

void on_agora_on_reconnecting_fn(GstElement* object,
                              gchararray userName,
							  gint reason,
                              gpointer user_data){

    g_print("->Signal Test: on reconnecting: userid=%s, reason=%d\n", userName, reason);
}

void on_agora_on_reconnected_fn(GstElement* object,
                              gchararray userName,
							  gint reason,
                              gpointer user_data){

    g_print("->Signal Test: on reconnected: userid=%s, reason=%d\n", userName, reason);
}

void on_agora_on_disconnected_fn(GstElement* object,
                                 gchararray userName,
							     gint reason,
                                 gpointer user_data){

    g_print("->Signal Test: on disconnected: userid=%s, reason=%d\n", userName, reason);
}


void on_agora_on_uplink_info_updated_fn(GstElement* object,
										guint video_encoder_target_bitrate_bps,
                                        gpointer user_data){

    g_print("->Signal Test: uplink info updated: target bps %d\n", video_encoder_target_bitrate_bps);
}

void on_agora_on_connection_lost_fn(GstElement* object,gpointer user_data){

    g_print("->Signal Test: connection lost!\n");
}

void on_agora_on_video_subscribed_fn(GstElement* object,
                                    gchararray userName,
									gpointer user_data){

    g_print("->Signal Test: on video subscribed, userid: %s\n", userName);
}

void on_remote_track_stats_fn(GstElement* object,
                              gchararray userName,
					          guint receivedBitrate,
							  guint decoderOutputFrameRate,
							  guint rendererOutputFrameRate,
							  guint frameLossRate,
							  guint packetLossRate,
							  guint rxStreamType,

							  guint totalFrozenTime,
							  guint frozenRate,
							  guint totalDecodedFrames,

							  guint avSyncTimeMs,
							  guint downlink_process_time_ms,
							  guint frame_render_delay_ms,

					          gpointer user_data){

    g_print("->Signal Test: remote stats for user %s: ", userName);
	g_print("receivedBitrate: %d, ", receivedBitrate);
	g_print("decoderOutputFrameRate: %d, ", decoderOutputFrameRate);
	g_print("rendererOutputFrameRate: %d, ",rendererOutputFrameRate);
	g_print("frameLossRate: %d, ", frameLossRate);
	g_print("packetLossRate: %d, ",packetLossRate);
	g_print("rxStreamType: %d, ",rxStreamType);
	g_print("totalFrozenTime: %d, ",totalFrozenTime);
	g_print("frozenRate: %d, ",frozenRate);
	g_print("totalDecodedFrames: %d, ",totalDecodedFrames);
	g_print("avSyncTimeMs: %d, ", avSyncTimeMs);
	g_print("downlink_process_time_ms: %d, ", downlink_process_time_ms);
	g_print("frame_render_delay_ms: %d\n",frame_render_delay_ms);
}

void on_local_track_stats_fn(GstElement* object,
                            gchararray userName,
							guint number_of_streams,
							guint bytes_major_stream,
							guint bytes_minor_stream,
							guint frames_encoded,
							guint ssrc_major_stream,
							guint ssrc_minor_stream,
							guint input_frame_rate,
							guint encode_frame_rate,
							guint render_frame_rate,
							guint target_media_bitrate_bps,
							guint media_bitrate_bps,
							guint total_bitrate_bps,
							guint width,
							guint height,
							guint encoder_type,
							gpointer user_data){

	g_print("->Signal Test: local stats for user %s: \n", userName);
	g_print("number_of_streams: %d, ", number_of_streams);	
	g_print("bytes_major_stream: %d, ",bytes_major_stream);
    g_print("bytes_minor_stream: %d, ",bytes_minor_stream);
	g_print("frames_encoded: %d, ", frames_encoded);
	g_print("input_frame_rate: %d, ",input_frame_rate);
	g_print("encode_frame_rate: %d, ",encode_frame_rate);
	g_print("render_frame_rate: %d, ",render_frame_rate);
	g_print("target_media_bitrate_bps: %d, ",target_media_bitrate_bps);
	g_print("media_bitrate_bps: %d, ",media_bitrate_bps);
	g_print("total_bitrate_bps: %d, ",total_bitrate_bps);
	g_print("width: %d, ", width);
	g_print("height: %d, ",height);
	g_print("encoder_type: %d \n", encoder_type);
}
void on_agora_on_user_state_changed_fn(GstElement* object,
                                       gchararray userName,
							           gint newState,
                                       gpointer user_data){

 enum State{

   USER_STATE_JOIN=1,
   USER_STATE_LEAVE,
   USER_STATE_CAM_ON,
   USER_STATE_CAM_OFF,
   USER_STATE_MIC_ON,
   USER_STATE_MIC_OFF
};
     switch (newState){
		 case USER_STATE_JOIN:
		      g_print("->Endtest: on user state changed: userid=%s, state: join\n", userName);
		      break;
		case USER_STATE_LEAVE:
		       g_print("->Endtest: on user state changed: userid=%s, state: leave\n", userName);
		      break;
		case USER_STATE_CAM_ON:
		      g_print("->Endtest: on user state changed: userid=%s, state: cam on\n", userName);
		      break;
		case USER_STATE_CAM_OFF:
		      g_print("->Endtest: on user state changed: userid=%s, state: cam off\n", userName);
		      break;
		case USER_STATE_MIC_ON:
		      g_print("->Endtest: on user state changed: userid=%s, state: mic on\n", userName);
		      break;
		case USER_STATE_MIC_OFF:
		      g_print("->Endtest: on user state changed: userid=%s, state: mic off\n", userName);
		      break;
		default:
		     break;
	 }
}

void print_usage(char* name){
	g_print("usage: %s appid channel\n", name);
}
int main(int argc, char *argv[]) {


  if(argc<3){
	  print_usage(argv[0]);
	  return 0;
  }

  setenv("GST_PLUGIN_PATH","/usr/local/lib/x86_64-linux-gnu/gstreamer-1.0",1);

  char buffer[MAX_BUFFER];

  GstElement *pipeline;

  GstBus *bus;

  GstMessage *msg;

  /* Initialize GStreamer */

  gst_init (&argc, &argv);

  /* Build the pipeline */

  char* appid=argv[1];
  char* channel=argv[2];

  snprintf (buffer, MAX_BUFFER, "videotestsrc pattern=ball is-live=true ! video/x-raw,format=I420,width=320,height=180,framerate=60/1 ! videoconvert ! x264enc key-int-max=60 tune=zerolatency ! agoraioudp appid=%s channel=%s userid=123 inport=7374 verbose=false ! fakesink", appid, channel);

  pipeline = gst_parse_launch (
     buffer
	,NULL);

  /* Start playing */


  g_object_set (pipeline, "message-forward", TRUE, NULL);

  GstElement *agoraioUdp = gst_bin_get_by_name(GST_BIN(pipeline), "agoraioudp0");
  if(agoraioUdp!=NULL)
  {
		g_print (">>installing signals to agoraio\n");

		//you may not need all of them 
		//on iframe request
		g_signal_connect (agoraioUdp, "on-iframe",
                    G_CALLBACK (on_agora_iframe_request_fn), NULL);

        //on connecting
		g_signal_connect (agoraioUdp, "on-connecting",
                    G_CALLBACK (on_agora_on_connecting_fn), NULL);

		//on connected
		g_signal_connect (agoraioUdp, "on-connected",
                    G_CALLBACK (on_agora_on_connected_fn), NULL);

		//on reconnecting
		g_signal_connect (agoraioUdp, "on-reconnecting",
                    G_CALLBACK (on_agora_on_connecting_fn), NULL);

		//on reconnected
		g_signal_connect (agoraioUdp, "on-reconnected",
                    G_CALLBACK (on_agora_on_connected_fn), NULL);
		
        //on user state changed 
		g_signal_connect (agoraioUdp, "on-user-state-changed",
                    G_CALLBACK (on_agora_on_user_state_changed_fn), NULL);

		//on uplink info updated
		g_signal_connect (agoraioUdp, "on-uplink-network-changed",
                    G_CALLBACK (on_agora_on_uplink_info_updated_fn), NULL);


		//on connection lost 
		g_signal_connect (agoraioUdp, "on-connection-lost",
                    G_CALLBACK (on_agora_on_connection_lost_fn), NULL);

		//on video subscribed 
		g_signal_connect (agoraioUdp, "on-user-video-subscribed",
                    G_CALLBACK (on_agora_on_video_subscribed_fn), NULL);

		//on remote track stats changed
		g_signal_connect (agoraioUdp, "on-remote-track-stats",
                    G_CALLBACK (on_remote_track_stats_fn), NULL);

		//on local track stats changed
		g_signal_connect (agoraioUdp, "on-local-track-stats",
                    G_CALLBACK (on_local_track_stats_fn), NULL);

  }
  

  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  /* Wait until error or EOS */

  bus = gst_element_get_bus (pipeline);

  msg = NULL;

  int exit = 0;

  while(!msg && exit==0) {

	msg = gst_bus_timed_pop_filtered (bus, 1000*1000*1000, GST_MESSAGE_INFO|GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

	if (msg == NULL) {

		// Timed out. Send an EOS after N secs to end the call

		static int secs = 1;

		if (secs == 20) {

		        gst_element_set_state(pipeline, GST_STATE_PAUSED);

			g_print("Pause pipe\n");

		}

		if (secs == 40) {

		        gst_element_set_state(pipeline, GST_STATE_PLAYING);

			g_print("Resume pipe\n");

		}

		if (secs == 60) {

		        gst_element_send_event(pipeline, gst_event_new_eos());
				

			g_print("EOS sent\n");

		}

		g_print("time - %d\n",secs++);

		continue;

	}

	// Handle valid exit messages

	switch (GST_MESSAGE_TYPE(msg)) {

		case GST_MESSAGE_ERROR:

			g_print("msg: error\n");

			exit = 1;

			break;

		case GST_MESSAGE_EOS:

			g_print("msg: eos\n");

			exit = 1;

			break;

		default:

			g_print("msg: unexpected message (%d)\n",msg->type);

			msg= NULL;

			break;

	}

  }

  /* Free resources */

  if (msg != NULL)

    gst_message_unref (msg);

  gst_object_unref (bus);

  gst_element_set_state (pipeline, GST_STATE_NULL);

  gst_object_unref (pipeline);

  gst_deinit();

  g_print("Clean exit\n");

  return 0;

}
