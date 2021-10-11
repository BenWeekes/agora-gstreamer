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
  PROP_SILENT,
  APP_ID,
  CHANNEL_ID,
  USER_ID,
  AUDIO
};

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );


//#define gst_agorasrc_parent_class parent_class
G_DEFINE_TYPE (Gstagorasrc, gst_agorasrc, GST_TYPE_PUSH_SRC);

static void gst_agorasrc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_agorasrc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);


int init_agora(Gstagorasrc * src){

   if (strlen(src->app_id)==0){
       g_print("app id cannot be empty!\n");
       return -1;
   }

   if (strlen(src->channel_id)==0){
       g_print("channel id cannot be empty!\n");
       return -1;
   }

    /*initialize agora*/
   src->agora_ctx=agora_receive_init(src->app_id,src->channel_id, src->user_id);
   if(src->agora_ctx==NULL){

      g_print("agora COULD NOT  be initialized\n");
      return -1;   
   }

   g_print("agora has been successfuly initialized\n");
  

   return 0;
}

static GstFlowReturn
gst_video_test_src_fill (GstPushSrc * psrc, GstBuffer * buffer){
   
  const size_t  max_size=4*1024*1024;
  int is_key_frame=0;
  size_t data_size=0;
  size_t in_buffer_size=0;

  GstMemory *memory=NULL;

  Gstagorasrc *agoraSrc = GST_AGORASRC (psrc);

  unsigned char* data=malloc(max_size);
  if(data==NULL){
     g_print("cannot allocate memory\n");
     return GST_FLOW_ERROR;
  }

  if(agoraSrc->audio==FALSE){
     data_size=get_next_video_frame(agoraSrc->agora_ctx, data, max_size, &is_key_frame);
  }
  else{
    data_size=get_next_audio_frame(agoraSrc->agora_ctx, data, max_size);
  }

  in_buffer_size=gst_buffer_get_size (buffer);

  /*increase the buffer if it is less than the frame data size*/
  if(data_size>in_buffer_size){
    memory = gst_allocator_alloc (NULL, (data_size-in_buffer_size), NULL);
    gst_buffer_insert_memory (buffer, -1, memory);
  }

  gst_buffer_fill(buffer, 0, data, data_size);
  gst_buffer_set_size(buffer, data_size);

  free(data);

   return GST_FLOW_OK;
}

static gboolean
gst_video_test_src_start (GstBaseSrc * basesrc)
{
  Gstagorasrc *src = GST_AGORASRC (basesrc);

  GST_OBJECT_LOCK (src);
 
  if(src->agora_ctx==NULL && init_agora(src)!=0){
     g_print("cannot initialize agora\n");
     return GST_FLOW_ERROR;
  }

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

  gstpushsrc_class->fill = gst_video_test_src_fill;
  gstbasesrc_class->start = gst_video_test_src_start;
  

  gobject_class->set_property = gst_agorasrc_set_property;
  gobject_class->get_property = gst_agorasrc_get_property;

 g_object_class_install_property (gobject_class, PROP_SILENT,
      g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
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
  
  agoraSrc->silent = FALSE;
  agoraSrc->audio=FALSE;
}
static void
gst_agorasrc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  Gstagorasrc *agoraSrc = GST_AGORASRC (object);

  const gchar* str;

  switch (prop_id) {
    case PROP_SILENT:
      agoraSrc->silent = g_value_get_boolean (value);
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
    case PROP_SILENT:
      g_value_set_boolean (value, agoraSrc->silent);
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
     

