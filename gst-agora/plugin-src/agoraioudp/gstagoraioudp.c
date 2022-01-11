/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2021 Ubuntu <<user@hostname.org>>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
* Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-agoraioudp
 *
 * FIXME:Describe agoraioudp here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! agoraioudp ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>

#include <gst/base/gstbasesrc.h>
#include <glib/gstdio.h>

#include "gstagoraioudp.h"

GST_DEBUG_CATEGORY_STATIC (gst_agoraioudp_debug);
#define GST_CAT_DEFAULT gst_agoraioudp_debug

/* Filter signals and args */
enum
{
  ON_IFRAME_SIGNAL=1,
  ON_CONNECTING_SIGNAL,
  ON_CONNECTED_SIGNAL,
  ON_DISCONNECTED_SIGNAL,
  ON_USER_STATE_CHANGED_SIGNAL,
  ON_UPLINK_NETWORK_INFO_UPDATED_SIGNAL,

  ON_CONNECTION_LOST_SIGNAL,
  ON_CONNECTION_FAILURE_SIGNAL,

  ON_RECONNECTING_SIGNAL,
  ON_RECONNECTED_SIGNAL,

  ON_VIDEO_SUBSCRIBED_SIGNAL,

  ON_REMOTE_TRACK_STATS_CHANGED,
  ON_LOCAL_TRACK_STATS_CHANGED,

  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_VERBOSE,
  APP_ID,
  CHANNEL_ID,
  USER_ID,
  IN_PORT,
  OUT_PORT,
  HOST,
  AUDIO,
  IN_AUDIO_DELAY,
  IN_VIDEO_DELAY,
  OUT_AUDIO_DELAY,
  OUT_VIDEO_DELAY
};


static guint agoraio_signals[LAST_SIGNAL] = { 0 };


/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );


#define gst_agoraioudp_parent_class parent_class
G_DEFINE_TYPE (Gstagoraioudp, gst_agoraioudp, GST_TYPE_ELEMENT);

static void gst_agoraioudp_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_agoraioudp_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static void handle_video_out_fn(const u_int8_t* buffer, u_int64_t len, void* user_data );

static void handle_audio_out_fn(const u_int8_t* buffer, u_int64_t len, void* user_data );


/*static void on_request_audio_data (GstAppSrc *appsrc, guint unused_size,
                           gpointer    user_data)
{   
    static GstClockTime timestamp = 0;
    GstBuffer *buffer;
    GstFlowReturn ret;
    size_t data_size=0;

    size_t  max_data_size=960*2*2;  //two channels of 48000 sample rate

    AgoraIoContext_t* agora_ctx=(AgoraIoContext_t*)user_data;

    gpointer recv_data=malloc(max_data_size);
    data_size=agoraio_read_audio(agora_ctx, recv_data, max_data_size);

    buffer = gst_buffer_new_allocate (NULL, data_size, NULL);
    gst_buffer_fill(buffer, 0, recv_data, data_size);
    gst_buffer_set_size(buffer, data_size);

    timestamp += GST_BUFFER_DURATION (buffer);

    //g_signal_emit_by_name (appsrc, "push-buffer", buffer, &ret);
    ret = gst_app_src_push_buffer(appsrc, buffer);

    if (ret != GST_FLOW_OK) {
       // g_print("not able to push audio data\n");
         something wrong, stop pushing 
    }
}*/

/* The appsink has received a buffer */
static GstFlowReturn new_sample (GstElement *sink, gpointer *user_data) {
  
   GstSample *sample;
   AgoraIoContext_t* agora_ctx=(AgoraIoContext_t*)user_data;

  /* Retrieve the buffer */
  g_signal_emit_by_name (sink, "pull-sample", &sample);
  if (sample) 
  {
    GstBuffer * in_buffer=gst_sample_get_buffer (sample);

    size_t data_size=gst_buffer_get_size (in_buffer);
    gpointer data=malloc(data_size);
    if(data==NULL){
       g_print("cannot allocate memory!\n");
       return GST_FLOW_ERROR;
    }

    if (agora_ctx==NULL) {
         return GST_FLOW_ERROR;
    }

    gst_buffer_extract(in_buffer,0, data, data_size);

    GstClockTime in_buffer_pts= GST_BUFFER_CAST(in_buffer)->pts;
    /*if(agoraIO->verbose){
         g_print("audio timestamp: %ld\n",(long)(in_buffer_pts/1000000));
    }*/
    
    agoraio_send_audio(agora_ctx, data, data_size,(in_buffer_pts)/1000000);

    free(data);


    gst_sample_unref (sample);
    return GST_FLOW_OK;
  }

  return GST_FLOW_ERROR;
}

void handle_agora_pending_events(Gstagoraioudp *agoraIO, 
                                 int eventType,
                                 const char* userName,
                                 long param1, 
                                 long param2,
                                 long* states);

static void handle_event_Signal(void* userData, 
                         int type, 
                         const char* userName,
                         long param1,
                         long param2,
                         long* states){

  
    Gstagoraioudp* agoraIO=(Gstagoraioudp*)(userData);

    handle_agora_pending_events(agoraIO, type, userName,param1, param2, states);

}
int init_agora(Gstagoraioudp *agoraIO){

   if (strlen(agoraIO->app_id)==0){
       g_print("app id cannot be empty!\n");
       return -1;
   }

   if (strlen(agoraIO->channel_id)==0){
       g_print("channel id cannot be empty!\n");
       return -1;
   }

    /*initialize agora*/
   agoraIO->agora_ctx=agoraio_init(agoraIO->app_id,  /*appid*/
                                agoraIO->channel_id, /*channel*/
                                agoraIO->user_id,    /*user id*/
                                 FALSE,             /*is audio user*/
                                 0,                 /*enable encryption */
                                 0,                 /*enable dual */
                                 500000,            /*dual video bitrate*/
                                 320,               /*dual video width*/
                                 180,               /*dual video height*/
                                 12,                /*initial size of video buffer*/
                                 30,                /*dual fps*/
                                 agoraIO->verbose,  /*log level*/
                                 handle_event_Signal, /*signal function to call*/
                                (void*)(agoraIO),      /*additional params to the signal function*/ 
                                 agoraIO->in_audio_delay,
                                 agoraIO->in_video_delay,
                                 agoraIO->out_audio_delay,
                                 agoraIO->out_video_delay,
                                 0);                     /*send only flag*/
         

   if(agoraIO->agora_ctx==NULL){

      g_print("agora COULD NOT  be initialized\n");
      return FALSE;   
   }

   //this function will be called whenever there is a video frame ready 
   agoraio_set_video_out_handler(agoraIO->agora_ctx, handle_video_out_fn, (void*)(agoraIO));
   agoraio_set_audio_out_handler(agoraIO->agora_ctx, handle_audio_out_fn, (void*)(agoraIO));

   //initialize timestamps to zero
   agoraIO->video_ts=0;
   agoraIO->audio_ts=0;

  
   g_print("agora has been successfuly initialized\n");

   agoraIO->appAudioSrc= gst_element_factory_make ("appsrc", "source");
   if(!agoraIO->appAudioSrc){
       g_print("failed to create audio app src\n");
   }
   else{
       g_print("created audio app src successfully\n");
   }
   agoraIO->udpsink = gst_element_factory_make("udpsink", "udpsink");
   if(!agoraIO->udpsink){
       g_print("failed to create audio udpsink\n");
   }
   else{
       g_print("created udpsink successfully\n");
   }

   agoraIO->udpsrc = gst_element_factory_make("udpsrc", "udpsrc");
   if(!agoraIO->udpsrc){
       g_print("failed to create audio udpsrc\n");
   }
   else{
       g_print("created udpsrc successfully\n");
   }

   agoraIO->appAudioSink = gst_element_factory_make("appsink", "appsink");
   if(!agoraIO->appAudioSink){
       g_print("failed to create audio appsink\n");
   }
   else{
       g_print("created appsink successfully\n");
   }

   agoraIO->out_pipeline = gst_pipeline_new ("pipeline");
   if(!agoraIO->out_pipeline){
       g_print("failed to create audio pipeline\n");
   }

   agoraIO->in_pipeline = gst_pipeline_new ("in-pipeline");
   if(!agoraIO->in_pipeline){
       g_print("failed to create audio pipeline\n");
   }

   //out plugin
   gst_bin_add_many (GST_BIN (agoraIO->out_pipeline), agoraIO->appAudioSrc, agoraIO->udpsink, NULL);
   gst_element_link_many (agoraIO->appAudioSrc, agoraIO->udpsink, NULL);

   //in plugin
   gst_bin_add_many (GST_BIN (agoraIO->in_pipeline),agoraIO->udpsrc, agoraIO->appAudioSink, NULL);
   gst_element_link_many (agoraIO->udpsrc, agoraIO->appAudioSink, NULL);


    //setup appsrc 
    g_object_set (G_OBJECT (agoraIO->appAudioSrc),
            "stream-type", 0,
            "is-live", TRUE,
            "format", GST_FORMAT_TIME, NULL);

     g_object_set (G_OBJECT (agoraIO->udpsink),
            "host", agoraIO->host,
            "port", agoraIO->out_port,
              NULL);

    g_object_set (G_OBJECT (agoraIO->udpsrc),
             "port", agoraIO->in_port,
              NULL);

    //agoraIO->cbs.need_data = on_request_audio_data;
    //gst_app_src_set_callbacks(GST_APP_SRC_CAST(agoraIO->appAudioSrc),
    //                             &agoraIO->cbs, agoraIO->agora_ctx, NULL);

    //set the pipeline in playing mode
    gst_element_set_state (agoraIO->out_pipeline, GST_STATE_PLAYING);
    gst_element_set_state (agoraIO->in_pipeline, GST_STATE_PLAYING);

    //Configure appsink 
    g_object_set (agoraIO->appAudioSink, "emit-signals", TRUE, NULL);
    g_signal_connect (agoraIO->appAudioSink, "new-sample",
                    G_CALLBACK (new_sample), agoraIO->agora_ctx);

  
   return TRUE;
}

void print_packet(u_int8_t* data, int size){
  
  int i=0;

  for(i=0;i<size;i++){
    printf(" %d ", data[i]);
  }

  printf("\n================================\n");
    
}

void handle_agora_pending_events(Gstagoraioudp *agoraIO, 
                                 int eventType,
                                 const char* userName,
                                 long param1, 
                                 long param2,
                                 long* states){

       switch (eventType){
             case ON_IFRAME_SIGNAL: 
                  g_signal_emit (G_OBJECT (agoraIO),agoraio_signals[ON_IFRAME_SIGNAL], 0);
                  break;
             case ON_CONNECTING_SIGNAL: 
                  g_signal_emit (G_OBJECT (agoraIO),agoraio_signals[ON_CONNECTING_SIGNAL], 0);
                  break;
             case ON_CONNECTED_SIGNAL: 
                  g_signal_emit (G_OBJECT (agoraIO),agoraio_signals[ON_CONNECTED_SIGNAL], 0, userName,param1);
                  break;
              case ON_DISCONNECTED_SIGNAL: 
                  g_signal_emit (G_OBJECT (agoraIO),agoraio_signals[ON_DISCONNECTED_SIGNAL], 0, userName,param1);
                  break;
             case ON_UPLINK_NETWORK_INFO_UPDATED_SIGNAL: 
                  g_signal_emit (G_OBJECT (agoraIO),agoraio_signals[ON_UPLINK_NETWORK_INFO_UPDATED_SIGNAL], 0, param1);
                  break;
             case ON_CONNECTION_LOST_SIGNAL: 
                  g_signal_emit (G_OBJECT (agoraIO),agoraio_signals[ON_CONNECTION_LOST_SIGNAL], 0);
                  break;
             case ON_CONNECTION_FAILURE_SIGNAL: 
                  g_signal_emit (G_OBJECT (agoraIO),agoraio_signals[ON_CONNECTION_FAILURE_SIGNAL], 0);
                  break;
            case ON_RECONNECTING_SIGNAL: 
                  g_signal_emit (G_OBJECT (agoraIO),agoraio_signals[ON_RECONNECTING_SIGNAL], 0,userName,param1);
                  break;
            case ON_RECONNECTED_SIGNAL: 
                  g_signal_emit (G_OBJECT (agoraIO),agoraio_signals[ON_RECONNECTED_SIGNAL], 0,userName,param1);
                  break;
            case ON_USER_STATE_CHANGED_SIGNAL: 
                  g_signal_emit (G_OBJECT (agoraIO),agoraio_signals[ON_USER_STATE_CHANGED_SIGNAL], 0,userName,param1);
                  break;
            case ON_VIDEO_SUBSCRIBED_SIGNAL: 
                  g_signal_emit (G_OBJECT (agoraIO),agoraio_signals[ON_VIDEO_SUBSCRIBED_SIGNAL], 0,userName);
                  break;
            case ON_REMOTE_TRACK_STATS_CHANGED: 
                  g_signal_emit (G_OBJECT (agoraIO),agoraio_signals[ON_REMOTE_TRACK_STATS_CHANGED], 0, userName,
                                 states[0], states[1], states[2],
                                 states[3], states[4], states[5],
                                 states[6], states[7], states[8],
                                 states[9], states[10], states[11]);

                  break;
             case ON_LOCAL_TRACK_STATS_CHANGED: 
                  g_signal_emit (G_OBJECT (agoraIO),agoraio_signals[ON_LOCAL_TRACK_STATS_CHANGED], 0, userName,
                                 states[0], states[1], states[2],
                                 states[3], states[4], states[5],
                                 states[6], states[7], states[8],
                                 states[9], states[10], states[11],
                                 states[12], states[13], states[14]);

                  break;
            default:
                 return; //may be there is no more signals 
       }
}

//handle video out from agora to the plugin
static void handle_video_out_fn(const u_int8_t* buffer, u_int64_t len, void* user_data ){

    Gstagoraioudp* agoraIO=(Gstagoraioudp*)(user_data);

    GstPad * peer=gst_pad_get_peer(agoraIO->srcpad);
    if(peer==NULL){
        g_print("handle_video_out_fn: unexpected error. Cannot reach peer pad\n");
        return;
    }

     GstBuffer * out_buffer=gst_buffer_new_allocate (NULL, len, NULL);

     gst_buffer_fill(out_buffer, 0, buffer, len);
     gst_buffer_set_size(out_buffer, len);


     //GST_BUFFER_CAST(out_buffer)->pts=in_buffer_pts;
     //GST_BUFFER_CAST(out_buffer)->dts=30000000//in_buffer_dts;
     GST_BUFFER_CAST(out_buffer)->duration=30000000;

     GstFlowReturn retCode=gst_pad_push (agoraIO->srcpad, out_buffer);
     if(retCode!=GST_FLOW_OK){
         g_print("cannot push video to sync\n");
     }

     gst_object_unref(peer);

}

static void handle_audio_out_fn(const u_int8_t* data_buffer, u_int64_t len, void* user_data ){

    GstBuffer *buffer;
    GstFlowReturn ret;
    
    Gstagoraioudp* agoraIO=(Gstagoraioudp*)(user_data);

    buffer = gst_buffer_new_allocate (NULL, len, NULL);
    gst_buffer_fill(buffer, 0, data_buffer, len);
    gst_buffer_set_size(buffer, len);

    GST_BUFFER_CAST(buffer)->pts=agoraIO->audio_ts;
    GST_BUFFER_CAST(buffer)->dts=agoraIO->audio_ts;
    GST_BUFFER_CAST(buffer)->duration=GST_BUFFER_DURATION (buffer);

    agoraIO->audio_ts += GST_BUFFER_DURATION (buffer);
    

    ret = gst_app_src_push_buffer(GST_APP_SRC_CAST(agoraIO->appAudioSrc), buffer);
    if (ret != GST_FLOW_OK) {
        g_print("handle_audio_out_fn: not able to push audio data\n");
        /* something wrong, stop pushing */
    }
}

static GstFlowReturn gst_agoraio_chain (GstPad * pad, GstObject * parent, GstBuffer * in_buffer){

    
    size_t data_size=0;
    int    is_key_frame=0;

    Gstagoraioudp *agoraIO=GST_AGORAIOUDP (parent);

    //do nothing in case of pause
    if(agoraIO->state==PAUSED){
        return GST_FLOW_OK;
    }

    data_size=gst_buffer_get_size (in_buffer);
    GstClockTime in_buffer_pts= GST_BUFFER_CAST(in_buffer)->pts;
  
    gpointer data=malloc(data_size);
    if(data==NULL){
      gst_buffer_unref (in_buffer);
       g_print("cannot allocate memory!\n");
       return GST_FLOW_ERROR;
    }

    gst_buffer_extract(in_buffer,0, data, data_size);
     if(GST_BUFFER_FLAG_IS_SET(in_buffer, GST_BUFFER_FLAG_DELTA_UNIT) == FALSE){
        is_key_frame=1;
     }

     if(agoraIO->audio==FALSE){
        unsigned long ts=(unsigned long)(in_buffer_pts/1000000); //in ms
        agoraio_send_video(agoraIO->agora_ctx, data, data_size,is_key_frame,ts);
      }

     free(data); 
     gst_buffer_unref (in_buffer);

    return GST_FLOW_OK;
}


static GstStateChangeReturn
gst_on_change_state (GstElement *element, GstStateChange transition)
{

  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;

  Gstagoraioudp *agoraIO=GST_AGORAIOUDP (element);

  g_print("AgoraIO: state change request from the plugin: \n");
  switch (transition) {
       case GST_STATE_CHANGE_NULL_TO_READY:
            g_print("AgoraIO: state change: NULL to READY \n");   
            if(agoraIO->agora_ctx==NULL && init_agora(agoraIO)!=TRUE){
                g_print("cannot initialize agora\n");
                return GST_FLOW_ERROR;
             }
            break;
	   case GST_STATE_CHANGE_READY_TO_NULL:
	        g_print("AgoraIO: state change: READY to NULL\n");
	        break;
       case GST_STATE_CHANGE_READY_TO_PAUSED:
            g_print("AgoraIO: state change: READY to PAUSED\n");
            break;
       case GST_STATE_CHANGE_PAUSED_TO_READY:
            g_print("AgoraIO: state change: PAUSED to READY \n");
            break;
       case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
            agoraIO->state=PAUSED;
            if(agoraIO->agora_ctx!=NULL)
                agoraio_set_paused(agoraIO->agora_ctx, TRUE);
            g_print("AgoraIO: state change: PLAYING to PAUSED \n");
            break;
        case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
            agoraIO->state=RUNNING;
            if(agoraIO->agora_ctx!=NULL)
                agoraio_set_paused(agoraIO->agora_ctx, FALSE);
            g_print("AgoraIO: state change: PAUSED to PLAYING  \n");
            break;
	   default:
            g_print("AgoraIO: unknown state change\n");  
	        break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  
  return ret;
}

/* this function handles sink events */
static gboolean
gst_agoraio_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  Gstagoraioudp *agoraIO;

  g_print("AgoraIO: received an event from the pipe: %d \n", GST_EVENT_TYPE (event));

  agoraIO = GST_AGORAIOUDP (parent);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
       {
        GstCaps * caps;
        gst_event_parse_caps (event, &caps);
       }
       break;
    case GST_EVENT_EOS:
        agoraIO->state=ENDED;
        gst_element_send_event(agoraIO->in_pipeline, gst_event_new_eos());
        gst_element_send_event(agoraIO->out_pipeline, gst_event_new_eos());
        
        if(!gst_element_set_state (agoraIO->in_pipeline, GST_STATE_NULL) ||
           !gst_element_set_state (agoraIO->out_pipeline, GST_STATE_NULL)){
               g_print("not able to stop audio plugins!\n");
        }

        agoraio_disconnect(&agoraIO->agora_ctx);
        agoraIO->agora_ctx=NULL;

        //release internal pipelines
        gst_object_unref (agoraIO->in_pipeline);
        gst_object_unref (agoraIO->out_pipeline);

        break;
    default:
      break;
  }

  return  gst_pad_event_default (pad, parent, event);
}

/* GObject vmethod implementations */

/* initialize the agoraioudp's class */
static void
gst_agoraioudp_class_init (GstagoraioudpClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_agoraioudp_set_property;
  gobject_class->get_property = gst_agoraioudp_get_property;

  //on pipeline stae change
  gstelement_class->change_state = gst_on_change_state;

 g_object_class_install_property (gobject_class, PROP_VERBOSE,
      g_param_spec_boolean ("verbose", "verbose", "Produce verbose output ?",
          FALSE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, AUDIO,
      g_param_spec_boolean ("audio", "audio", "when true, it reads audio from agora than video",
          FALSE, G_PARAM_READWRITE));

  /*app id*/
  g_object_class_install_property (gobject_class, APP_ID,
      g_param_spec_string ("appid", "appid", "agora app id",
          FALSE, G_PARAM_READWRITE));

  /*channel_id*/
  g_object_class_install_property (gobject_class, CHANNEL_ID,
      g_param_spec_string ("channel", "channel", "agora channel id",
          FALSE, G_PARAM_READWRITE));

  /*user_id*/
  g_object_class_install_property (gobject_class, USER_ID,
      g_param_spec_string ("userid", "userid", "agora user id (optional)",
          FALSE, G_PARAM_READWRITE));


  /*in port*/
  g_object_class_install_property (gobject_class, IN_PORT,
      g_param_spec_int ("inport", "inport", "udp port that we receive audio on it",0, G_MAXUINT16,
          5004, G_PARAM_READWRITE));
    

  /*out port*/
  g_object_class_install_property (gobject_class, OUT_PORT,
      g_param_spec_int ("outport", "outport", "udp port that we send audio on it", 0, G_MAXUINT16,
          5004, G_PARAM_READWRITE));

  /*host*/
  g_object_class_install_property (gobject_class, HOST,
      g_param_spec_string ("host", "host", "udp host that we send audio to it",
          FALSE, G_PARAM_READWRITE));

  /*in audio delay*/
  g_object_class_install_property (gobject_class, IN_AUDIO_DELAY,
      g_param_spec_int ("in-audio-delay", "in-audio-delay", "amount of delay (ms) for audio gst -> agora SDK", 0, G_MAXUINT16,
          0, G_PARAM_READWRITE));

  /*in video delay*/
  g_object_class_install_property (gobject_class, IN_VIDEO_DELAY,
      g_param_spec_int ("in-video-delay", "in-video-delay", "amount of delay (ms) for video from gst -> agora SDK ", 0, G_MAXUINT16,
          0, G_PARAM_READWRITE));
     
  /*in audio delay*/
  g_object_class_install_property (gobject_class, OUT_AUDIO_DELAY,
      g_param_spec_int ("out-audio-delay", "out-audio-delay", "amount of delay (ms) for audio from agora SDK -> gst ", 0, G_MAXUINT16,
          0, G_PARAM_READWRITE));

   /*in video delay*/
  g_object_class_install_property (gobject_class, OUT_VIDEO_DELAY,
      g_param_spec_int ("out-video-delay", "out-video-delay", "amount of delay (ms) for video from agora SDK -> gst ", 0, G_MAXUINT16,
          0, G_PARAM_READWRITE));

  gst_element_class_set_details_simple(gstelement_class,
    "agorasrc",
    "agorasrc",
    "read h264 from agora and send it to the child",
    "Ben <<benweekes73@gmail.com>>");

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_factory));

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_factory));


  /*install agoraio available signals*/
  agoraio_signals[ON_IFRAME_SIGNAL] =
      g_signal_new ("on-iframe", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,G_TYPE_NONE, 0);

  agoraio_signals[ON_CONNECTING_SIGNAL] =
      g_signal_new ("on-connecting", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,G_TYPE_NONE, 0);

    
  agoraio_signals[ON_CONNECTED_SIGNAL] =
      g_signal_new ("on-connected", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,G_TYPE_NONE,2, G_TYPE_STRING, G_TYPE_INT);

  agoraio_signals[ON_DISCONNECTED_SIGNAL] =
      g_signal_new ("on-disconnected", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,G_TYPE_NONE,2, G_TYPE_STRING, G_TYPE_INT);

 agoraio_signals[ON_UPLINK_NETWORK_INFO_UPDATED_SIGNAL] =
      g_signal_new ("on-uplink-network-changed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,G_TYPE_NONE, 1, G_TYPE_INT);

 agoraio_signals[ON_CONNECTION_LOST_SIGNAL] =
      g_signal_new ("on-connection-lost", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,G_TYPE_NONE, 0);

 agoraio_signals[ON_CONNECTION_FAILURE_SIGNAL] =
      g_signal_new ("on-connection-failure", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,G_TYPE_NONE, 0);

 agoraio_signals[ON_RECONNECTING_SIGNAL] =
      g_signal_new ("on-reconnecting", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,G_TYPE_NONE,2, G_TYPE_STRING, G_TYPE_INT);

 agoraio_signals[ON_RECONNECTED_SIGNAL] =
      g_signal_new ("on-reconnected", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,G_TYPE_NONE,2, G_TYPE_STRING, G_TYPE_INT);

 agoraio_signals[ON_USER_STATE_CHANGED_SIGNAL] =
      g_signal_new ("on-user-state-changed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_INT);

 agoraio_signals[ON_VIDEO_SUBSCRIBED_SIGNAL] =
      g_signal_new ("on-user-video-subscribed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,G_TYPE_NONE, 1, G_TYPE_STRING);

 agoraio_signals[ON_REMOTE_TRACK_STATS_CHANGED] =
      g_signal_new ("on-remote-track-stats", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,G_TYPE_NONE, 
       13, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT,
                         G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT,
                         G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT,
                         G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT);


  agoraio_signals[ON_LOCAL_TRACK_STATS_CHANGED] =
      g_signal_new ("on-local-track-stats", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,G_TYPE_NONE, 
       16, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT,
                         G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT,
                         G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT,
                         G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT,
                          G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT);

}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_agoraioudp_init (Gstagoraioudp * agoraIO)
{
  //for src
  agoraIO->srcpad = gst_pad_new_from_static_template (&src_factory, "src");

  GST_PAD_SET_PROXY_CAPS (agoraIO->srcpad);
  gst_element_add_pad (GST_ELEMENT (agoraIO), agoraIO->srcpad);

  //for sink 
  agoraIO->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_chain_function (agoraIO->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_agoraio_chain));

  gst_pad_set_event_function (agoraIO->sinkpad,gst_agoraio_sink_event);

  GST_PAD_SET_PROXY_CAPS (agoraIO->sinkpad);
  gst_element_add_pad (GST_ELEMENT (agoraIO), agoraIO->sinkpad);


  //set it initially to null
  agoraIO->agora_ctx=NULL;
   
  //set app_id and channel_id to zero
  memset(agoraIO->app_id, 0, MAX_STRING_LEN);
  memset(agoraIO->channel_id, 0, MAX_STRING_LEN);
  memset(agoraIO->user_id, 0, MAX_STRING_LEN);

  memset(agoraIO->host, 0, MAX_STRING_LEN);
  strcpy(agoraIO->host,"127.0.0.1");

  agoraIO->in_port=5004;
  agoraIO->out_port=5004;
  
  agoraIO->verbose = FALSE;
  agoraIO->audio=FALSE;
}

static void
gst_agoraioudp_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{

 Gstagoraioudp *agoraIO = GST_AGORAIOUDP (object);

 const gchar* str;

  switch (prop_id) {
    case PROP_VERBOSE:
         agoraIO->verbose = g_value_get_boolean (value);
         break;
    case APP_ID:
        str=g_value_get_string (value);
        g_strlcpy(agoraIO->app_id, str, MAX_STRING_LEN);
        break;
    case CHANNEL_ID:
        str=g_value_get_string (value);
        g_strlcpy(agoraIO->channel_id, str, MAX_STRING_LEN);
        break; 
    case USER_ID:
        str=g_value_get_string (value);
        g_strlcpy(agoraIO->user_id, str, MAX_STRING_LEN);
        break; 
    case AUDIO: 
        agoraIO->audio = g_value_get_boolean (value);
        break;
    case IN_PORT: 
       agoraIO->in_port=g_value_get_int (value);
       break;
    case OUT_PORT: 
       agoraIO->out_port=g_value_get_int (value);
       break;
    case HOST: 
       str=g_value_get_string (value);
       g_strlcpy(agoraIO->host, str, MAX_STRING_LEN);
       break;
    case IN_AUDIO_DELAY: 
       agoraIO->in_audio_delay=g_value_get_int (value);
       break;
    case IN_VIDEO_DELAY: 
       agoraIO->in_video_delay=g_value_get_int (value);
       break;
    case OUT_AUDIO_DELAY: 
       agoraIO->out_audio_delay=g_value_get_int (value);
       break;
    case OUT_VIDEO_DELAY: 
       agoraIO->out_video_delay=g_value_get_int (value);
       break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
  }
}

static void
gst_agoraioudp_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  Gstagoraioudp *agoraIO = GST_AGORAIOUDP (object);

  switch (prop_id) {
    case PROP_VERBOSE:
       g_value_set_boolean (value, agoraIO->verbose);
       break;
    case APP_ID:
       g_value_set_string (value, agoraIO->app_id);
       break;
    case CHANNEL_ID:
        g_value_set_string (value, agoraIO->channel_id);
       break;
    case USER_ID:
        g_value_set_string (value, agoraIO->user_id);
        break;
    case AUDIO:
        g_value_set_boolean (value, agoraIO->audio);
        break;
    case IN_PORT:
        g_value_set_int (value, agoraIO->in_port);
        break;
    case OUT_PORT:
        g_value_set_int (value, agoraIO->out_port);
        break;
    case HOST:
        g_value_set_string (value, agoraIO->host);
        break;
    case IN_AUDIO_DELAY: 
        g_value_set_int(value, agoraIO->in_audio_delay);
        break;
    case IN_VIDEO_DELAY: 
        g_value_set_int(value, agoraIO->in_video_delay);
        break;
    case OUT_AUDIO_DELAY: 
        g_value_set_int(value, agoraIO->out_audio_delay);
        break;
    case OUT_VIDEO_DELAY: 
        g_value_set_int(value, agoraIO->out_video_delay);
       break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
agoraioudp_init (GstPlugin * agoraioudp)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template agoraioudp' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_agoraioudp_debug, "agoraioudp",
      0, "Template agoraioudp");

  return gst_element_register (agoraioudp, "agoraioudp", GST_RANK_NONE,
      GST_TYPE_AGORAIOUDP);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "agoraioudp"
#endif

/* gstreamer looks for this structure to register agoraioudps
 *
 * exchange the string 'Template agoraioudp' with your agoraioudp description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    agoraioudp,
    "agoraioudp",
    agoraioudp_init,
    PACKAGE_VERSION,
    GST_LICENSE,
    GST_PACKAGE_NAME,
    GST_PACKAGE_ORIGIN
)
     

