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


#define gst_agoraioudp_parent_class parent_class
G_DEFINE_TYPE (Gstagoraioudp, gst_agoraioudp, GST_TYPE_ELEMENT);

static void gst_agoraioudp_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_agoraioudp_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static GstFlowReturn gst_agoraio_src_fill (GstPushSrc * psrc, GstBuffer * buffer){

   // g_print("gst_agoraio_src_fill\n");
    
    return GST_FLOW_OK;
}

static void on_request_audio_data (GstAppSrc *appsrc, guint unused_size,
                           gpointer    user_data)
{
    //g_print("In %s\n", __func__);
   
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

    gst_buffer_extract(in_buffer,0, data, data_size);

    agoraio_send_audio(agora_ctx, data, data_size,0);

    free(data);

    gst_sample_unref (sample);
    return GST_FLOW_OK;
  }

  return GST_FLOW_ERROR;
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
                                 agoraIO->verbose);               

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

    /* setup appsrc */
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
    //agoraIO->cbs.enough_data = cb_enough_data;
    //agoraIO->cbs.seek_data = cb_seek_data;

    gst_app_src_set_callbacks(GST_APP_SRC_CAST(agoraIO->appAudioSrc),
                                 &agoraIO->cbs, agoraIO->agora_ctx, NULL);
    gst_element_set_state (agoraIO->out_pipeline, GST_STATE_PLAYING);

    /* Configure appsink */
    g_object_set (agoraIO->appAudioSink, "emit-signals", TRUE, NULL);
    g_signal_connect (agoraIO->appAudioSink, "new-sample",
                    G_CALLBACK (new_sample), agoraIO->agora_ctx);

    gst_element_set_state (agoraIO->in_pipeline, GST_STATE_PLAYING);

   return TRUE;
}

void print_packet(u_int8_t* data, int size){
  
  int i=0;

  for(i=0;i<size;i++){
    printf(" %d ", data[i]);
  }

  printf("\n================================\n");
    
}

static GstFlowReturn gst_agoraio_chain (GstPad * pad, GstObject * parent, GstBuffer * in_buffer){

    size_t data_size=0;
    int    is_key_frame=0;

    size_t in_buffer_size=0;

    GstMemory *memory=NULL;

    Gstagoraioudp *agoraIO=GST_AGORAIOUDP (parent);

   //TODO: we need a better position to initialize agora. 
   //gst_agorasink_init() is good, however, it is called before reading app and channels ids
   if(agoraIO->agora_ctx==NULL && init_agora(agoraIO)!=TRUE){
      g_print("cannot initialize agora\n");
      return GST_FLOW_ERROR;
    }

    data_size=gst_buffer_get_size (in_buffer);
  
    gpointer data=malloc(data_size);
    if(data==NULL){
       g_print("cannot allocate memory!\n");
       return GST_FLOW_ERROR;
    }

    gst_buffer_extract(in_buffer,0, data, data_size);
     if(GST_BUFFER_FLAG_IS_SET(in_buffer, GST_BUFFER_FLAG_DELTA_UNIT) == FALSE){
        is_key_frame=1;
     }

     if(agoraIO->audio==FALSE){
        agoraio_send_video(agoraIO->agora_ctx, data, data_size,is_key_frame, agoraIO->ts);
      }
     else{
        //agoraio_send_audio(agoraIO->agora_ctx, data, data_size, agoraIO->ts);
      }
 
     if (agoraIO->verbose == true){
        g_print ("agorasink: received %" G_GSIZE_FORMAT" bytes!\n",data_size);
        print_packet(data, 10);
        g_print("agorasink: is key frame: %d\n", is_key_frame);
     }

     free(data); 
     agoraIO->ts+=30;

     GstPad * peer=gst_pad_get_peer(agoraIO->srcpad);
     if(peer==NULL){
         return GST_FLOW_OK;
     }

     //receive data
     const size_t  max_size=4*1024*1024;
     unsigned char* recvData=malloc(max_size);
     if(recvData==NULL){
        g_print("cannot allocate memory\n");
        return GST_FLOW_ERROR;
     }

     data_size=agoraio_read_video(agoraIO->agora_ctx, recvData, max_size, &is_key_frame);
     if(data_size==0){
         free(recvData);
         return GST_FLOW_OK;
     }

    in_buffer_size=gst_buffer_get_size (in_buffer);

     /*increase the buffer if it is less than the frame data size*/
    if(data_size>in_buffer_size){
       memory = gst_allocator_alloc (NULL, (data_size-in_buffer_size), NULL);
       gst_buffer_insert_memory (in_buffer, -1, memory);
     }

     gst_buffer_fill(in_buffer, 0, recvData, data_size);
     gst_buffer_set_size(in_buffer, data_size);

     free(recvData);

    return gst_pad_push (agoraIO->srcpad, in_buffer);
}


/* GObject vmethod implementations */

/* initialize the agoraioudp's class */
static void
gst_agoraioudp_class_init (GstagoraioudpClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

 // GstBaseSrcClass *gstbasesrc_class;
  GstPushSrcClass *gstpushsrc_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  //gstbasesrc_class = (GstBaseSrcClass *) klass;
  gstpushsrc_class = (GstPushSrcClass *) klass;

  gstpushsrc_class->fill = gst_agoraio_src_fill;
  //gstbasesrc_class->start = gst_video_test_src_start;  

  gobject_class->set_property = gst_agoraioudp_set_property;
  gobject_class->get_property = gst_agoraioudp_get_property;

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


  gst_element_class_set_details_simple(gstelement_class,
    "agorasrc",
    "agorasrc",
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
gst_agoraioudp_init (Gstagoraioudp * agoraIO)
{
  //gst_base_src_set_live (GST_BASE_SRC (agoraIO), TRUE);
  //gst_base_src_set_blocksize  (GST_BASE_SRC (agoraIO), 10*1024);

  //for src
  agoraIO->srcpad = gst_pad_new_from_static_template (&src_factory, "src");

  GST_PAD_SET_PROXY_CAPS (agoraIO->srcpad);
  gst_element_add_pad (GST_ELEMENT (agoraIO), agoraIO->srcpad);

  //for sink 
  agoraIO->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_chain_function (agoraIO->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_agoraio_chain));

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
     

