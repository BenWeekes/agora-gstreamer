#include <gst/gst.h>

int main(int argc, char *argv[]) {

  GstElement *pipeline;

  GstBus *bus;

  GstMessage *msg;

  /* Initialize GStreamer */

  gst_init (&argc, &argv);

  /* Build the pipeline */

  pipeline = gst_parse_launch (

	"videotestsrc pattern=ball is-live=true ! video/x-raw,format=I420,width=320,height=180,framerate=60/1 ! videoconvert ! x264enc key-int-max=60 tune=zerolatency ! agoraioudp appid=20b7c51ff4c644ab80cf5a4e646b0537 channel=test userid=123 ! fakesink sync=false"
	//" videotestsrc pattern=ball is-live=true ! video/x-raw,format=I420,width=320,height=180,framerate=60/1 ! videoconvert ! x264enc key-int-max=60 tune=zerolatency ! filesink location=test.h264"
     //"audiotestsrc num-buffers=1000 ! fakesink sync=false"
	 // "videotestsrc pattern=ball is-live=true ! video/x-raw,format=I420,width=320,height=180,framerate=60/1 ! videoconvert ! agoraio ! fakesink sync=false"
	 //"videotestsrc pattern=ball is-live=true ! video/x-raw,format=I420,width=320,height=180,framerate=60/1 ! videoconvert ! x264enc key-int-max=60 tune=zerolatency ! agoraioudp appid=20b7c51ff4c644ab80cf5a4e646b0537 channel=test userid=123 ! fakesink sync=false"
	 //"videotestsrc pattern=ball is-live=true ! video/x-raw,format=I420,width=320,height=180,framerate=60/1 ! videoconvert ! x264enc key-int-max=60 tune=zerolatency ! agoraio appid=20b7c51ff4c644ab80cf5a4e646b0537 channel=test userid=123 ! fakesink sync=false"
	// Without audio

//	"v4l2src device=/dev/video0 ! "

//	"agoraioudp  appid=ba68d9ec60fb45cb94254e0b4d01af92 channel=forme3 verbose=true ! "

//	"queue ! h264parse ! vaapidecodebin ! "

//	"autovideosink "

	// Test with audio

	/*"v4l2src device=/dev/video0 ! video/x-h264,width=1440,height=1920,format=byte-stream,framerate=[30/1,10000000/333333] ! queue ! "

	"agoraioudp  appid=ba68d9ec60fb45cb94254e0b4d01af92 channel=forme3 outport=7372 inport=7373 verbose=true ! "

	"queue ! h264parse ! vaapidecodebin ! autovideosink "

 	"alsasrc device=sysdefault:CARD=CX20921 name=micsrc ! audio/x-raw,width=16,depth=16,rate=44100,channel=1 ! queue leaky=2 max-size-time=100000000 ! audioconvert ! audioresample quality=8 ! opusenc ! audio/x-opus,rate=48000,channels=1 ! udpsink host=127.0.0.1 port=7373 "

        "udpsrc port=7372 ! audio/x-raw,format=S16LE,channels=1,rate=48000,layout=interleaved ! audioconvert ! queue name=1on1AudIn ! pulsesink volume=1.0 name=incaudsink"*/

	,NULL);

  /* Start playing */


  g_object_set (pipeline, "message-forward", TRUE, NULL);
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

		if (secs == 1) {

		        gst_element_set_state(pipeline, GST_STATE_PAUSED);

			g_print("Pause pipe\n");

		}

		if (secs == 2) {

		        gst_element_set_state(pipeline, GST_STATE_PLAYING);

			g_print("Resume pipe\n");

		}

		if (secs == 3) {

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


