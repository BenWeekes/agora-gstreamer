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
  char audio_out_pipe_str[MAX_BUFFER/4];
  char audio_in_pipe_str[MAX_BUFFER/4];
  char complete_pipe_str[MAX_BUFFER];

  /* Initialize GStreamer */
  gst_init (&argc, &argv);

  char* appid=argv[1];
  char* channel=argv[2];

  snprintf (video_pipe_str, MAX_BUFFER/4, "v4l2src ! image/jpeg,width=640,height=360 ! jpegdec ! queue ! videoconvert ! x264enc key-int-max=30 tune=zerolatency ! queue  ! agoraioudp appid=%s channel=%s outport=7372 inport=7373 in-audio-delay=30 in-video-delay=100 verbose=false ! queue ! decodebin ! queue ! glimagesink sync=false", appid, channel);

  snprintf (audio_in_pipe_str, MAX_BUFFER/4, "udpsrc port=7372 ! audio/x-raw,format=S16LE,channels=1,rate=48000,layout=interleaved ! audioconvert ! queue name=1on1AudIn ! pulsesink  name=incaudsink");

  snprintf (audio_out_pipe_str, MAX_BUFFER/4,"alsasrc ! audio/x-raw,format=S16LE,channels=1,rate=48000 ! queue leaky=2 max-size-time=100000000 ! audioconvert ! audioresample quality=8 ! opusenc ! audio/x-opus,rate=48000,channels=1 ! udpsink host=127.0.0.1 port=7373");

  snprintf (complete_pipe_str, MAX_BUFFER,"%s %s %s",video_pipe_str, audio_out_pipe_str, audio_in_pipe_str);

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
		static int secs = 1;

		if (secs == 20) {
		  gst_element_set_state(pipeline, GST_STATE_PAUSED);
			g_print("Pause pipe\n");
		}

		if (secs == 30) {
		  gst_element_set_state(pipeline, GST_STATE_PLAYING);
			g_print("Resume pipe\n");
		}

		if (secs == 40) {
		  gst_element_send_event(pipeline, gst_event_new_eos());
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
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);

  gst_deinit();

  system("sleep 1");
  g_print("----- Open files after gst deinit\n");
  system("lsof | grep video0");

  g_print("exit\n");
  return 0;
}

