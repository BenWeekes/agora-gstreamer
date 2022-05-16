#include <gst/gst.h> 

#include <stdio.h>

#include "common.h"

#define MAX_BUFFER 1024*5

void print_usage(char* name){
	g_print("usage: %s appid channel\n", name);
}

int main(int argc, char *argv[]) {
  GstElement *pipeline;

  GstElement *audio_in_pipeline;
  GstElement *audio_out_pipeline;
  GstElement *video_pipeline;

  GstBus *bus;
  GstMessage *msg;

  if(argc<3){
	  print_usage(argv[0]);
	  return 0;
  }

  setenv("GST_PLUGIN_PATH","/usr/local/lib/x86_64-linux-gnu/gstreamer-1.0",1);
  
  char video_pipe_str[MAX_BUFFER/4];
  char audio_out_pipe_str[MAX_BUFFER/4];
  char audio_in_pipe_str[MAX_BUFFER/4];
  char complete_pipe_str[MAX_BUFFER];

  /* Initialize GStreamer */
  gst_init (&argc, &argv);

  char* appid=argv[1];
  char* channel=argv[2];

  int mode=3;
  if(argc>3){
     mode=atoi(argv[3]);
  }

  snprintf (video_pipe_str, MAX_BUFFER/4, "gst-launch-1.0 -v videotestsrc pattern=ball is-live=true ! video/x-raw,format=I420,width=320,height=180,framerate=60/1 ! videoconvert ! x264enc key-int-max=60 tune=zerolatency ! agoraioudp appid=%s channel=%s userid=123 inport=7374 verbose=false  ! filesink location=nn.h264", appid, channel);
  snprintf (audio_in_pipe_str, MAX_BUFFER/4, "udpsrc port=7372 ! audio/x-raw,format=S16LE,channels=1,rate=48000,layout=interleaved ! audioconvert ! queue name=1on1AudIn ! pulsesink  name=incaudsink");
  
  //in mode 1, we send raw audio directly 
  if(mode==1){
      snprintf (audio_out_pipe_str, MAX_BUFFER/4,"alsasrc ! audio/x-raw,format=S16LE,channels=1,rate=48000 ! audioconvert ! udpsink host=127.0.0.1 port=7373");
  }
  else{
      snprintf (audio_out_pipe_str, MAX_BUFFER/4,"alsasrc ! audio/x-raw,format=S16LE,channels=1,rate=48000 ! queue leaky=2 max-size-time=100000000 ! audioconvert ! audioresample quality=8 ! opusenc ! audio/x-opus,rate=48000,channels=1 ! udpsink host=127.0.0.1 port=7373");
  }

  //snprintf (complete_pipe_str, MAX_BUFFER,"%s %s %s",video_pipe_str, audio_out_pipe_str, audio_in_pipe_str);

  //launch three pipes
  video_pipeline = gst_parse_launch (video_pipe_str,NULL);
  audio_in_pipeline = gst_parse_launch (audio_in_pipe_str,NULL);
  audio_out_pipeline = gst_parse_launch (audio_out_pipe_str,NULL);

  /* Start playing */
  gst_element_set_state (video_pipeline, GST_STATE_PLAYING);
  gst_element_set_state (audio_in_pipeline, GST_STATE_PLAYING);
  gst_element_set_state (audio_out_pipeline, GST_STATE_PLAYING);


  /* we may only monitor the video pipe as audio ones are just slaves to it */
  bus = gst_element_get_bus (video_pipeline);

  msg = NULL;
  int exit = 0;
  while(!msg && exit==0) {
	msg = gst_bus_timed_pop_filtered (bus, 1000*1000*1000, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

	if (msg == NULL) {
		// Send an EOS after N secs to end the call
		static int secs = 1;
		if (secs == 3600) {
		    gst_element_send_event(video_pipeline, gst_event_new_eos());
        gst_element_send_event(audio_in_pipeline, gst_event_new_eos());
        gst_element_send_event(audio_out_pipeline, gst_event_new_eos());
			  g_print("EOS sent\n");
		}

		g_print("time - %d\n",secs++);
		continue;
	}

  GError *err;
	gchar *d;
	
	// Handle valid exit messages
	switch (GST_MESSAGE_TYPE(msg)) {
		case GST_MESSAGE_ERROR:
			g_print("msg: error\n");
      gst_message_parse_error(msg, &err, &d);
      g_print("Error: %d message=%s\n", err->code, err->message);
      g_print("Debug: %s\n", d);
      g_free(d);
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

  gst_element_set_state (video_pipeline, GST_STATE_NULL);
  gst_element_set_state (audio_in_pipeline, GST_STATE_NULL);
  gst_element_set_state (audio_out_pipeline, GST_STATE_NULL);

  gst_object_unref (video_pipeline);
  gst_object_unref (audio_in_pipeline);
  gst_object_unref (audio_out_pipeline);

  gst_deinit();

  system("sleep 5");
  g_print("----- Open files after gst deinit\n");
  system("lsof | grep video0");

  g_print("exit\n");
  return 0;
}

