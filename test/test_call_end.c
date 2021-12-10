#include <gst/gst.h> 

int main(int argc, char *argv[]) {
  GstElement *pipeline;
  GstBus *bus;
  GstMessage *msg;

  /* Initialize GStreamer */
  gst_init (&argc, &argv);

  /* Build the pipeline */
  pipeline = gst_parse_launch (

	"videotestsrc pattern=ball is-live=true ! video/x-raw,format=I420,width=320,height=180,framerate=60/1 ! videoconvert ! x264enc key-int-max=60 tune=zerolatency ! agoraioudp appid=xxx channel=xxx userid=123 ! fakesink sync=false"
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
		// Send an EOS after N secs to end the call
		static int secs = 1;
		if (secs == 5) {
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

  system("sleep 5");
  g_print("----- Open files after gst deinit\n");
  system("lsof | grep video0");

  g_print("exit\n");
  return 0;
}

