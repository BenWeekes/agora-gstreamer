#include <gst/gst.h> 

#include <stdio.h>

#define MAX_BUFFER 1024*5

void print_usage(char* name){
	g_print("usage: %s appid channel\n", name);
}

int main(int argc, char *argv[]) {
  GstElement *pipeline;
  GstBus *bus;
  GstMessage *msg;

   if(argc<3){
	  print_usage(argv[0]);
	  return 0;
  }

  setenv("GST_PLUGIN_PATH","/usr/local/lib/x86_64-linux-gnu/gstreamer-1.0",1);

  //signal(SIGINT, on_exit_signal);

  char video_pipe_str[MAX_BUFFER/4];
  char audio_in_pipe_str[MAX_BUFFER/4];
  char complete_pipe_str[MAX_BUFFER];

  /* Initialize GStreamer */
  gst_init (&argc, &argv);

  char* appid=argv[1];
  char* channel=argv[2];

  snprintf (video_pipe_str, MAX_BUFFER/4, "videotestsrc pattern=ball is-live=true ! video/x-raw,format=I420,width=320,height=180,framerate=60/1 ! videoconvert ! x264enc key-int-max=60 tune=zerolatency ! agorasink  appid=%s channel=%s userid=123 inport=7373", appid, channel);
  snprintf (audio_in_pipe_str, MAX_BUFFER/4,"audiotestsrc wave=ticks apply-tick-ramp=true tick-interval=100000000 freq=10000 volume=0.4 marker-tick-period=10 sine-periods-per-tick=20  ! audio/x-raw,format=S16LE,channels=1,rate=16000,layout=interleaved ! audioconvert ! opusenc ! udpsink host=224.1.1.1 port=7373 auto-multicast=true");

  snprintf (complete_pipe_str, MAX_BUFFER,"%s %s ",video_pipe_str, audio_in_pipe_str);

  //snprintf (complete_pipe_str, MAX_BUFFER/4, " audiotestsrc wave=ticks apply-tick-ramp=true tick-interval=100000000 freq=10000 volume=0.4 marker-tick-period=10 sine-periods-per-tick=20  ! audio/x-raw,format=S16LE,channels=1,rate=16000,layout=interleaved ! audioconvert ! opusenc ! agorasink appid=%s channel=%s userid=123 audio=true", appid, channel);


  pipeline = gst_parse_launch (
     complete_pipe_str
	,NULL);

  /* Start playing */
  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  /* Wait until error or EOS */
  bus = gst_element_get_bus (pipeline);

  msg = NULL;
  int exit = 0;
  while(!msg && exit==0) {
	msg = gst_bus_timed_pop_filtered (bus, 1000*1000*1000, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

	if (msg == NULL) {
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

  g_print("exit\n");
  return 0;
}

