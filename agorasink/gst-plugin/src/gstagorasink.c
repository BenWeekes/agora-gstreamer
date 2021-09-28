/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2021 mostafa <<user@hostname.org>>
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
 */

/**
 * SECTION:element-agorasink
 *
 * a gstreamer plugin that forward a/v to agora
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! agorasink ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include "gstagorasink.h"
#include "agorah264parser.h"
#include <stdio.h>

GST_DEBUG_CATEGORY_STATIC (gst_agorasink_debug);
#define GST_CAT_DEFAULT gst_agorasink_debug

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_SILENT
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

#define gst_agorasink_parent_class parent_class
G_DEFINE_TYPE (Gstagorasink, gst_agorasink, GST_TYPE_ELEMENT);

static void gst_agorasink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_agorasink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_agorasink_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_agorasink_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);

/* GObject vmethod implementations */

/* initialize the agorasink's class */
static void
gst_agorasink_class_init (GstagorasinkClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_agorasink_set_property;
  gobject_class->get_property = gst_agorasink_get_property;

  g_object_class_install_property (gobject_class, PROP_SILENT,
      g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
          FALSE, G_PARAM_READWRITE));

  gst_element_class_set_details_simple(gstelement_class,
    "agorasink",
    "agorasink",
    "sends audio/video to an agora channel",
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
gst_agorasink_init (Gstagorasink * filter)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_agorasink_sink_event));
  gst_pad_set_chain_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_agorasink_chain));
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

   //initialize agora
   filter->agora_ctx=agora_init("20b7c51ff4c644ab80cf5a4e646b0537", "test","123", 0, 
                        0, 500000,
                         320,180,
                         12, 30);

   if(filter->agora_ctx==NULL){

      g_print("agora COULD NOT  be initialized\n");
      return;
       
   }

   g_print("agora has been successfuly initialized\n");
  
  int err=0;

  //init opus
  filter-> opus_encoder= opus_encoder_create(48000,1, OPUS_APPLICATION_VOIP, &err);
  if(err<0){
     g_print("cannot init opus\n");
     return ;
  }
  err = opus_encoder_ctl(filter-> opus_encoder, OPUS_SET_BITRATE(50000));
  if(err<0){
    g_print("cannot set opus bitrate\n");
     return ;
  }

  //set FEC
  opus_encoder_ctl(filter-> opus_encoder, OPUS_SET_INBAND_FEC(true));

  filter->silent = FALSE;
}

static void
gst_agorasink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  Gstagorasink *filter = GST_AGORASINK (object);

  switch (prop_id) {
    case PROP_SILENT:
      filter->silent = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_agorasink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  Gstagorasink *filter = GST_AGORASINK (object);

  switch (prop_id) {
    case PROP_SILENT:
      g_value_set_boolean (value, filter->silent);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstElement vmethod implementations */

/* this function handles sink events */
static gboolean
gst_agorasink_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  Gstagorasink *filter;
  gboolean ret;

  filter = GST_AGORASINK (parent);

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


int is_key_frame(u_int8_t* data){

  int fragment_type = data[0] & 0x1F;
  int nal_type = data[1] & 0x1F;
  int start_bit = data[1] & 0x80;

  if (((fragment_type == 28 || fragment_type == 29)
          && nal_type == 5 && start_bit == 128) ||
                fragment_type == 5){

          return 1;
    }

    return 0;
}

int check_key_frame(u_int8_t* in, int size){

   u_int8_t* currentBytes=in;

   int bytes=0;

   int latestNalPos=0;

   while(bytes<size-5){
      
      if(currentBytes[bytes] == 0
        && currentBytes[bytes+1] == 0
        && currentBytes[bytes+2] == 1){
             
            if(is_key_frame(&currentBytes[bytes+3])){
               printf("key frame\n");

               return 1;
             }
        }

        if(currentBytes[bytes] == 0
           && currentBytes[bytes+1] == 0
           && currentBytes[bytes+2] == 0
           && currentBytes[bytes+3] == 1){
          
          if(is_key_frame(&currentBytes[bytes+4])){
              return 1;
          }
        }

        bytes ++;
   }
   
   return 0;
}

long ts=0;

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_agorasink_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  Gstagorasink *filter;

  filter = GST_AGORASINK (parent);
  
  if (filter->silent == FALSE){

       /*g_print ("received %" G_GSIZE_FORMAT" bytes!\n",
        gst_buffer_get_size (buf));*/

      size_t buffer_size=gst_buffer_get_size (buf);
      gpointer data=malloc(buffer_size);
      gst_buffer_extract(buf,0, data, buffer_size);

      int is_key_frame=check_key_frame(data, buffer_size);
      //printf("is key frame: %d\n", is_key_frame);

      H264Frame frame;
      get_frame(data, buffer_size, &frame);

      printf("type: %d, start: %d, end: %d, data size: %ld\n",
              frame.is_key_frame, frame.start_position, 
              frame.end_position,  buffer_size);

      agora_send_video(filter->agora_ctx, data,buffer_size,frame.is_key_frame, ts);
      
      free(data);

      /*int length=960;
      for(int i=0;i<length;i++){
        filter->audio_buffer[i]=i;
      }
      int bytes=opus_encode(filter->opus_encoder,filter->audio_buffer,length, filter->out, length);
      //g_print ("encoded bytes: %d\n", bytes);

      agora_send_audio(filter->agora_ctx, filter->out, bytes, ts);*/

      ts+=30;
   
  }

  return GST_FLOW_OK;
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
agorasink_init (GstPlugin * agorasink)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template agorasink' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_agorasink_debug, "agorasink",
      0, "send audio/video to agora");

  return gst_element_register (agorasink, "agorasink", GST_RANK_NONE,
      GST_TYPE_AGORASINK);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "myfirstagorasink"
#endif

/* gstreamer looks for this structure to register agorasinks
 *
 * exchange the string 'Template agorasink' with your agorasink description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    agorasink,
    "agorasink",
    agorasink_init,
    PACKAGE_VERSION,
    GST_LICENSE,
    GST_PACKAGE_NAME,
    GST_PACKAGE_ORIGIN
)
