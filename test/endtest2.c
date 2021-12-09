#include <gst/gst.h>


void on_agora_iframe_request_fn(GstElement* object,gpointer user_data){

     g_print("->Endtest: iframe requested!\n");
}

void on_agora_on_connecting_fn(GstElement* object,gpointer user_data){

     g_print("->Endtest: on connecting!\n");
}

void on_agora_on_connected_fn(GstElement* object,gpointer user_data){

     g_print("->Endtest: on connected!\n");
}

int main(int argc, char *argv[]) {

  GstElement *pipeline;

  GstBus *bus;

  GstMessage *msg;

  /* Initialize GStreamer */

  gst_init (&argc, &argv);

  /* Build the pipeline */

  pipeline = gst_parse_launch (

	"videotestsrc pattern=ball is-live=true ! video/x-raw,format=I420,width=320,height=180,framerate=60/1 ! videoconvert ! x264enc key-int-max=60 tune=zerolatency ! agoraioudp appid=20b7c51ff4c644ab80cf5a4e646b0537 channel=test userid=123 ! fakesink sync=false"

	,NULL);

  /* Start playing */


  g_object_set (pipeline, "message-forward", TRUE, NULL);

  GstElement *agoraioUdp = gst_bin_get_by_name(GST_BIN(pipeline), "agoraioudp0");
  if(agoraioUdp!=NULL)
  {
		g_print (">>installing signals to agoraio\n");

		//you ma not need all of them 
		//on iframe request
		g_signal_connect (agoraioUdp, "on-iframe",
                    G_CALLBACK (on_agora_iframe_request_fn), NULL);

        //on connecting
		g_signal_connect (agoraioUdp, "on-connecting",
                    G_CALLBACK (on_agora_on_connecting_fn), NULL);

		//on connected
		g_signal_connect (agoraioUdp, "on-connected",
                    G_CALLBACK (on_agora_on_connected_fn), NULL);
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

		if (secs == 2) {

		        gst_element_set_state(pipeline, GST_STATE_PAUSED);

			g_print("Pause pipe\n");

		}

		if (secs == 4) {

		        gst_element_set_state(pipeline, GST_STATE_PLAYING);

			g_print("Resume pipe\n");

		}

		if (secs == 6) {

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


