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
 * SECTION:element-agoraio
 *
 * FIXME:Describe agoraio here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! agoraio ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>

#include <gst/base/gstbasesrc.h>
#include <glib/gstdio.h>

#include "gstagoraio.h"

GST_DEBUG_CATEGORY_STATIC (gst_agoraio_debug);
#define GST_CAT_DEFAULT gst_agoraio_debug

/* Filter signals and args */
enum
{
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
  AUDIO
};

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

#define gst_agoraio_parent_class parent_class
G_DEFINE_TYPE (Gstagoraio, gst_agoraio, GST_TYPE_ELEMENT);

static void gst_agoraio_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_agoraio_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_agoraio_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_agoraio_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);

/* GObject vmethod implementations */

int init_agora(Gstagoraio *agoraIO);

static GstStateChangeReturn
gst_on_change_state (GstElement *element, GstStateChange transition)
{

  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;

  Gstagoraio *agoraIO=GST_AGORAIO (element);

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
            //gst_element_set_state (agoraIO->out_pipeline, GST_STATE_PAUSED);
            //gst_element_set_state (agoraIO->in_pipeline, GST_STATE_PAUSED);
            agoraIO->state=PAUSED;
            agoraio_set_paused(agoraIO->agora_ctx, TRUE);
            g_print("AgoraIO: state change: PLAYING to PAUSED \n");
            break;
        case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
            /*gst_element_set_state (agoraIO->out_pipeline, GST_STATE_PLAYING);
            gst_element_set_state (agoraIO->in_pipeline, GST_STATE_PLAYING);
            agoraIO->state=RUNNING;
            agoraio_set_paused(agoraIO->agora_ctx, FALSE);*/
            g_print("AgoraIO: state change: PAUSED to PLAYING  \n");
            break;
	   default:
            g_print("AgoraIO: unknown state change\n");  
	        break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  
  return ret;
}

/* initialize the agoraio's class */
static void
gst_agoraio_class_init (GstagoraioClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_agoraio_set_property;
  gobject_class->get_property = gst_agoraio_get_property;

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

  gst_element_class_set_details_simple(gstelement_class,
    "agoraio",
    "FIXME:Generic",
    "read h264 from agora and send it to the child",
    "Ben <<benweekes73@gmail.com>>");

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_factory));
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_agoraio_init (Gstagoraio * filter)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_agoraio_sink_event));
  gst_pad_set_chain_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_agoraio_chain));
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

}
static void
gst_agoraio_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  Gstagoraio *filter = GST_AGORAIO (object);

  const gchar* str;

  switch (prop_id) {
    case PROP_VERBOSE:
      filter->verbose = g_value_get_boolean (value);
      break;
    case APP_ID:
        str=g_value_get_string (value);
        g_strlcpy(filter->app_id, str, MAX_STRING_LEN);
        break;
    case CHANNEL_ID:
        str=g_value_get_string (value);
        g_strlcpy(filter->channel_id, str, MAX_STRING_LEN);
        break; 
     case USER_ID:
        str=g_value_get_string (value);
        g_strlcpy(filter->user_id, str, MAX_STRING_LEN);
        break; 
     case AUDIO: 
        filter->audio = g_value_get_boolean (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
  }
}

static void
gst_agoraio_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  Gstagoraio *filter = GST_AGORAIO (object);

  switch (prop_id) {
    case PROP_VERBOSE:
       g_value_set_boolean (value, filter->verbose);
       break;
    case APP_ID:
       g_value_set_string (value, filter->app_id);
       break;
    case CHANNEL_ID:
        g_value_set_string (value, filter->channel_id);
       break;
    case USER_ID:
        g_value_set_string (value, filter->user_id);
        break;
    case AUDIO:
        g_value_set_boolean (value, filter->audio);
        break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}
/* GstElement vmethod implementations */

/* this function handles sink events */
static gboolean
gst_agoraio_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  Gstagoraio *filter;
  gboolean ret;

  filter = GST_AGORAIO (parent);

  GST_LOG_OBJECT (filter, "Received %s event: %" GST_PTR_FORMAT,
      GST_EVENT_TYPE_NAME (event), event);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps * caps;

      gst_event_parse_caps (event, &caps);
      /* do something with the caps */

      /* and forward */
      ret = gst_pad_event_default (pad, parent, event);
      break;
    }
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }
  return ret;
}

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_agoraio_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  /* just push out the incoming buffer without touching it */
  //return gst_pad_push (filter->srcpad, buf);
  return GST_FLOW_OK;
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
agoraio_init (GstPlugin * agoraio)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template agoraio' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_agoraio_debug, "agoraio",
      0, "Template agoraio");

  return gst_element_register (agoraio, "agoraio", GST_RANK_NONE,
      GST_TYPE_AGORAIO);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "agoraio"
#endif

/* gstreamer looks for this structure to register agoraios
 *
 * exchange the string 'Template agoraio' with your agoraio description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    agoraio,
    "agoraio",
    agoraio_init,
    PACKAGE_VERSION,
    GST_LICENSE,
    GST_PACKAGE_NAME,
    GST_PACKAGE_ORIGIN
)


static void on_request_audio_data (GstAppSrc *appsrc, guint unused_size,
                           gpointer    user_data)
{
    //g_print("In %s\n", __func__);
   
    static GstClockTime timestamp = 0;
    GstBuffer *buffer;
    GstFlowReturn ret;
    size_t data_size=0;

    size_t  max_data_size=960*2*2;  //two channels of 48000 sample rate

    //AgoraIoContext_t* agora_ctx=(AgoraIoContext_t*)user_data;

    gpointer recv_data=malloc(max_data_size);
    //data_size=agoraio_read_audio(agora_ctx, recv_data, max_data_size);

    buffer = gst_buffer_new_allocate (NULL, data_size, NULL);
    gst_buffer_fill(buffer, 0, recv_data, data_size);
    gst_buffer_set_size(buffer, data_size);

   // GST_BUFFER_PTS (buffer) = timestamp;
    //GST_BUFFER_DURATION (buffer) = gst_util_uint64_scale_int (1, GST_SECOND, 2);

    timestamp += GST_BUFFER_DURATION (buffer);

    //g_signal_emit_by_name (appsrc, "push-buffer", buffer, &ret);
    ret = gst_app_src_push_buffer(appsrc, buffer);

    if (ret != GST_FLOW_OK) {
        g_print("Error pushing data\n");
        /* something wrong, stop pushing */
    }
}

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

    agoraio_send_audio(agora_ctx, data, data_size,0);

    free(data);

    gst_sample_unref (sample);
    return GST_FLOW_OK;
  }

  return GST_FLOW_ERROR;
}


int init_agora(Gstagoraio *agoraIO){

   if (strlen(agoraIO->app_id)==0){
       g_print("app id cannot be empty!\n");
       return -1;
   }

   if (strlen(agoraIO->channel_id)==0){
       g_print("channel id cannot be empty!\n");
       return -1;
   }

    /*initialize agora*/
   agoraIO->agora_ctx=agoraio_init2(agoraIO->app_id,  /*appid*/
                                agoraIO->channel_id, /*channel*/
                                agoraIO->user_id,    /*user id*/
                                 FALSE,      /*is audio user*/
                                 0,                 /*enable encryption */
                                 0,                 /*enable dual */
                                 500000,            /*dual video bitrate*/
                                 320,               /*dual video width*/
                                 180,               /*dual video height*/
                                 12,                /*initial size of video buffer*/
                                 30,                 /*dual fps*/
                                 agoraIO->verbose,
                                 NULL,
                                 NULL,
                                 0,
                                 0,
                                 0,
                                 0);               

   if(agoraIO->agora_ctx==NULL){

      g_print("agora COULD NOT  be initialized\n");
      return FALSE;   
   }

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

    agoraIO->cbs.need_data = on_request_audio_data;
    
    gst_app_src_set_callbacks(GST_APP_SRC_CAST(agoraIO->appAudioSrc),
                                 &agoraIO->cbs, agoraIO->agora_ctx, NULL);

    //Configure appsink 
    g_object_set (agoraIO->appAudioSink, "emit-signals", TRUE, NULL);
    g_signal_connect (agoraIO->appAudioSink, "new-sample",
                    G_CALLBACK (new_sample), agoraIO->agora_ctx);

   
   return TRUE;
}

     

