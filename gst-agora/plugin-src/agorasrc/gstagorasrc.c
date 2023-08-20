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
 * SECTION:element-agorasrc
 *
 * FIXME:Describe agorasrc here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! agorasrc ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include <gst/base/gstbasesrc.h>
#include <glib/gstdio.h>
#include "gstagorasrc.h"

GST_DEBUG_CATEGORY_STATIC (gst_agorasrc_debug);
#define GST_CAT_DEFAULT gst_agorasrc_debug

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
  AUDIO,
  OUT_PORT,
  HOST
};

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    // GST_STATIC_CAPS (
    //     "video/x-raw,"
    //     "format=(string)I420,"
    //     "width=(int)[1,MAX],"
    //     "height=(int)[1,MAX]"
    // )
    );


//#define gst_agorasrc_parent_class parent_class
G_DEFINE_TYPE (Gstagorasrc, gst_agorasrc, GST_TYPE_PUSH_SRC);

static void gst_agorasrc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_agorasrc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

typedef struct{

   u_int8_t* data;
   size_t    len;
   bool decoded;
   int width;
   int height;
}Frame;

Frame* copy_frame(const u_int8_t* buffer, u_int64_t len){

     Frame* f=(Frame*)malloc(sizeof(Frame));
     if(f==NULL) return NULL;

     f->len=len;
     f->data=(u_int8_t*)malloc(len);
     
     if(f->data==NULL) {
       free(f);
       return NULL;
     } 

     memcpy(f->data, buffer, len);
     return f;
}
Frame* copy_decoded_frame(const u_int8_t* buffer, u_int64_t len,int width,int height){

     Frame* f=(Frame*)malloc(sizeof(Frame));
     if(f==NULL) return NULL;

     f->len=len;
     f->data=(u_int8_t*)malloc(len);
     
     if(f->data==NULL) {
       free(f);
       return NULL;
     } 

     memcpy(f->data, buffer, len);
     f->width = width;
     f->height = height;
     f->decoded = true;
     return f;
}

void destory_frame(Frame** f){

   free((*f)->data);

   free(*f);
}

//handle video out from agora to the plugin
static void handle_video_out_fn(const u_int8_t* buffer, u_int64_t len, void* user_data ){

    Gstagorasrc* agoraSrc=(Gstagorasrc*)(user_data);
    if(!agoraSrc->audio){

        Frame* f=copy_frame(buffer, len);
        g_queue_push_tail(agoraSrc->media_queue, f);
    }
}

static void handle_decoded_video_out_fn(const u_int8_t* buffer, u_int64_t len,int width,int height, void* user_data ){
    Gstagorasrc* agoraSrc=(Gstagorasrc*)(user_data);
    if(!agoraSrc->audio){
        Frame* f=copy_decoded_frame(buffer, len,width,height);
        g_queue_push_tail(agoraSrc->media_queue, f);
    }
}

static void handle_audio_out_fn(const u_int8_t* buffer, u_int64_t len, void* user_data ){

    Gstagorasrc* agoraSrc=(Gstagorasrc*)(user_data);
    if(agoraSrc->audio){
        Frame* f=copy_frame(buffer, len);
        g_queue_push_tail(agoraSrc->media_queue, f);
    }
}

int init_agora(Gstagorasrc * src){

   if (strlen(src->app_id)==0){
       g_print("app id cannot be empty!\n");
       return -1;
   }

   if (strlen(src->channel_id)==0){
       g_print("channel id cannot be empty!\n");
       return -1;
   }
    
   agora_config_t config;

   config.app_id=src->app_id;               /*appid*/
   config.ch_id=src->channel_id;            /*channel*/
   config.user_id=src->user_id;             /*user id*/
   config.is_audiouser=FALSE;                   /*is audio user*/
   config.enc_enable=0;                         /*enable encryption */
   config.enable_dual=0;                        /*enable dual */
   config.dual_vbr=500000;                      /*dual video bitrate*/
   config.dual_width=320;                       /*dual video width*/ 
   config.dual_height=180;                      /*dual video height*/
   config.min_video_jb=12;                      /*initial size of video buffer*/
   config.dfps=30;                              /*dual fps*/
   config.verbose=src->verbose;                 /*log level*/
   config.fn=NULL;                              /*signal function to call*/
   config.userData=(void*)(src);                /*additional params to the signal function*/ ;
   config.in_audio_delay=0;
   config.in_video_delay=0;
   config.out_audio_delay=0;
   config.out_video_delay=0;
   config.sendOnly= 0;                          /*send only flag*/
   config.enableProxy=FALSE;                    /*enable proxy*/
   config.proxy_timeout= 0;                     /*proxy timeout*/
   config.proxy_ips= "";                        /*proxy ips*/
   config.transcode=FALSE;                      /*proxy ips*/  


    /*initialize agora*/
   src->agora_ctx=agoraio_init(&config);    
  
   if(src->agora_ctx==NULL){

      g_print("agora COULD NOT  be initialized\n");
      return -1;   
   }

   //this function will be called whenever there is a video frame ready 
   agoraio_set_video_out_handler(src->agora_ctx, handle_video_out_fn, (void*)(src));
   agoraio_set_decoded_video_out_handler(src->agora_ctx, handle_decoded_video_out_fn, (void*)(src));
   agoraio_set_audio_out_handler(src->agora_ctx, handle_audio_out_fn, (void*)(src));

   //create a media queue
   src->media_queue=g_queue_new();

   g_print("agora has been successfuly initialized\n");
  
   src->out_pipeline = gst_pipeline_new ("pipeline");
   if(!src->out_pipeline){
       g_print("failed to create pipeline\n");
   }

   return 0;
}

Frame* get_next_frame(GQueue* q, int timeout)
{
  Frame* f=NULL;
  int sleep_per_wait=5000; //in micro seconds

  int current_wait_count=0;
  while((f=g_queue_pop_head (q))==NULL &&
         ((++current_wait_count)*sleep_per_wait)<timeout)
  {
     g_usleep(sleep_per_wait);
  }

  return f;
}
const int framerate = 30;
static GstFlowReturn
gst_media_test_src_fill (GstPushSrc * psrc, GstBuffer * buffer){
   
  //int is_key_frame=0;
  size_t data_size=0;
  GstMemory *memory=NULL;

  size_t in_buffer_size=0;

  Gstagorasrc *agoraSrc = GST_AGORASRC (psrc);
  if(agoraSrc->agora_ctx==NULL && init_agora(agoraSrc)!=0){
     g_print("cannot initialize agora\n");
     return GST_FLOW_ERROR;
   }
  //we wait for about 5 seconds before reporting an error
  Frame* f=get_next_frame(agoraSrc->media_queue, 5000000);
  if(f==NULL){
      return GST_FLOW_ERROR;
  }
  static int width,height;          
  if(f->decoded){
    if(width != f->width || height != f->height){
        GstState current_state, pending_state;
        gst_element_get_state(agoraSrc->out_pipeline, &current_state, &pending_state, GST_CLOCK_TIME_NONE);
        gst_element_set_state(agoraSrc->out_pipeline, GST_STATE_PAUSED);

        GstCaps* caps = gst_caps_new_simple("video/x-raw",
                                        "format", G_TYPE_STRING, "I420",
                                        "width", G_TYPE_INT, f->width,
                                        "height", G_TYPE_INT, f->height,
                                        "framerate", GST_TYPE_FRACTION, framerate, 1,
                                        NULL);
        // GstPad *srcpad = gst_element_get_static_pad(GST_ELEMENT(psrc), "src");
        // if(srcpad){
        //     gst_pad_set_caps(srcpad, caps);
        //     gst_object_unref(srcpad);
        // }
        gst_pad_set_caps(agoraSrc->srcpad, caps);
        gst_caps_unref(caps);
        gst_element_set_state(agoraSrc->out_pipeline, current_state);
    }
    width = f->width;
    height = f->height;
  }

  data_size=f->len;

  in_buffer_size=gst_buffer_get_size (buffer);

  //increase the buffer if it is less than the frame data size
  if(data_size>in_buffer_size){
    memory = gst_allocator_alloc (NULL, (data_size-in_buffer_size), NULL);
    gst_buffer_insert_memory (buffer, -1, memory);
  }

  gst_buffer_fill(buffer, 0, f->data, data_size);
  gst_buffer_set_size(buffer, data_size);

  static GstClockTime timestamp = 0;
  GstClockTime duration = GST_SECOND / framerate; 
  buffer->pts = timestamp;
  buffer->dts = GST_CLOCK_TIME_NONE;
  timestamp += duration;

  destory_frame(&f);

  if (agoraSrc->verbose == true){
     g_print ("agorasrc: sending %" G_GSIZE_FORMAT" bytes!\n",data_size);
  }

   return GST_FLOW_OK;
}

static gboolean
gst_media_test_src_start (GstBaseSrc * basesrc)
{
  Gstagorasrc *src = GST_AGORASRC (basesrc);

  GST_OBJECT_LOCK (src);


  GST_OBJECT_UNLOCK (src);

  return TRUE;
}

/* initialize the agorasrc's class */
static void
gst_agorasrc_class_init (GstagorasrcClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  GstBaseSrcClass *gstbasesrc_class;
  GstPushSrcClass *gstpushsrc_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gstbasesrc_class = (GstBaseSrcClass *) klass;
  gstpushsrc_class = (GstPushSrcClass *) klass;

  gstpushsrc_class->fill = gst_media_test_src_fill;
  gstbasesrc_class->start = gst_media_test_src_start;  

  gobject_class->set_property = gst_agorasrc_set_property;
  gobject_class->get_property = gst_agorasrc_get_property;

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
      g_param_spec_string ("remoteuserid", "remoteuserid", "agora user id to subscribe to it (optional)",
          FALSE, G_PARAM_READWRITE));

  /*out port*/
  g_object_class_install_property (gobject_class, OUT_PORT,
      g_param_spec_int ("outport", "outport", "outport udp port for audio out", 0, G_MAXUINT16,
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

}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_agorasrc_init (Gstagorasrc * agoraSrc)
{

  gst_base_src_set_live (GST_BASE_SRC (agoraSrc), TRUE);
  gst_base_src_set_blocksize  (GST_BASE_SRC (agoraSrc), 10*1024);

  //set it initially to null
  agoraSrc->agora_ctx=NULL;
   
  //set app_id and channel_id to zero
  memset(agoraSrc->app_id, 0, MAX_STRING_LEN);
  memset(agoraSrc->channel_id, 0, MAX_STRING_LEN);
  memset(agoraSrc->user_id, 0, MAX_STRING_LEN);
  
  agoraSrc->verbose = FALSE;
  agoraSrc->audio=FALSE;

  memset(agoraSrc->host, 0, MAX_STRING_LEN);
  strcpy(agoraSrc->host,"127.0.0.1");
  agoraSrc->out_port=5004;

  agoraSrc->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (agoraSrc->srcpad);
  gst_element_add_pad (GST_ELEMENT (agoraSrc), agoraSrc->srcpad);
}
static void
gst_agorasrc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  Gstagorasrc *agoraSrc = GST_AGORASRC (object);

  const gchar* str;

  switch (prop_id) {
    case PROP_VERBOSE:
      agoraSrc->verbose = g_value_get_boolean (value);
      break;
    case APP_ID:
        str=g_value_get_string (value);
        g_strlcpy(agoraSrc->app_id, str, MAX_STRING_LEN);
        break;
    case CHANNEL_ID:
        str=g_value_get_string (value);
        g_strlcpy(agoraSrc->channel_id, str, MAX_STRING_LEN);
        break; 
     case USER_ID:
        str=g_value_get_string (value);
        g_strlcpy(agoraSrc->user_id, str, MAX_STRING_LEN);
        break; 
     case AUDIO: 
        agoraSrc->audio = g_value_get_boolean (value);
        break;
     case OUT_PORT: 
        agoraSrc->out_port=g_value_get_int (value);
        break;
    case HOST: 
        str=g_value_get_string (value);
        g_strlcpy(agoraSrc->host, str, MAX_STRING_LEN);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
  }
}

static void
gst_agorasrc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  Gstagorasrc *agoraSrc = GST_AGORASRC (object);

  switch (prop_id) {
    case PROP_VERBOSE:
      g_value_set_boolean (value, agoraSrc->verbose);
      break;
    case APP_ID:
       g_value_set_string (value, agoraSrc->app_id);
       break;
    case CHANNEL_ID:
        g_value_set_string (value, agoraSrc->channel_id);
       break;
    case USER_ID:
        g_value_set_string (value, agoraSrc->user_id);
        break;
    case AUDIO:
        g_value_set_boolean (value, agoraSrc->audio);
        break;
    case OUT_PORT:
        g_value_set_int (value, agoraSrc->out_port);
        break;
    case HOST:
        g_value_set_string (value, agoraSrc->host);
        break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}
/* GstElement vmethod implementations */


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
agorasrc_init (GstPlugin * agorasrc)
{

  /* debug category for fltering log messages
   *
   * exchange the string 'Template agorasrc' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_agorasrc_debug, "agorasrc",
      0, "agorasrc");


  return gst_element_register (agorasrc, "agorasrc", GST_RANK_NONE,
      GST_TYPE_AGORASRC);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "agorasrc"
#endif

/* gstreamer looks for this structure to register agorasrcs
 *
 * exchange the string 'Template agorasrc' with your agorasrc description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    agorasrc,
    "agorasrc",
    agorasrc_init,
    PACKAGE_VERSION,
    GST_LICENSE,
    GST_PACKAGE_NAME,
    GST_PACKAGE_ORIGIN
)
     

