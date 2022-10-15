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
 * gst-launch-1.0 -v videotestsrc pattern=ball is-live=true ! video/x-raw,format=I420,width=320,height=180,framerate=60/1   ! videoconvert ! x264enc key-int-max=60 tune=zerolatency ! agorasink appid=id chid=ch
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include "gstagorasink.h"
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
  APP_ID,
  CHANNEL_ID,
  USER_ID,
  IN_PORT,
  PROP_VERBOSE,
  AUDIO,
  ENFORCE_AUDIO_DURAION,
  CONVERT_AVC_TO_ANNEX_B,
  TRANSCODE
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

  g_object_class_install_property (gobject_class, PROP_VERBOSE,
      g_param_spec_boolean ("verbose", "verbose", "Produce verbose output ?",
          FALSE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, TRANSCODE,
      g_param_spec_boolean ("transcode", "transcode", "Produce transcoded output as needed ?",
          FALSE, G_PARAM_READWRITE));

   g_object_class_install_property (gobject_class, AUDIO,
      g_param_spec_boolean ("audio", "audio", "when true, it sends audio to agora than video",
          FALSE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, ENFORCE_AUDIO_DURAION,
      g_param_spec_boolean ("enforce-audio-duration", "enforce-audio-duration", "when true, audio duration is enforced by agorasink",
          FALSE, G_PARAM_READWRITE));

   g_object_class_install_property (gobject_class, CONVERT_AVC_TO_ANNEX_B,
      g_param_spec_boolean ("avc-to-annexb", "avc-to-annexb", "when true, input h264 avc is converted to annexb format by agorasink",
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
      g_param_spec_int ("inport", "inport", "inport udp port for audio in",0, G_MAXUINT16,
          5004, G_PARAM_READWRITE));

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

  //set it initially to null
  filter->agora_ctx=NULL;
   
  //set app_id and channel_id to zero
  memset(filter->app_id, 0, MAX_STRING_LEN);
  memset(filter->channel_id, 0, MAX_STRING_LEN);
  memset(filter->user_id, 0, MAX_STRING_LEN);

  filter->verbose = FALSE;
  filter->transcode = FALSE;
  filter->audio = FALSE;

  filter->in_port=5004;

  filter->enforce_audio_duration=FALSE;

  filter->convert_avc_to_annexb=FALSE;

  filter->is_key_frame_seen=FALSE;

  filter->ts=0;
}

/* The appsink has received a buffer */
static GstFlowReturn new_sample (GstElement *sink, gpointer *user_data) {
  
   GstSample *sample;

   Gstagorasink *agoraSink=(Gstagorasink*)user_data;
   AgoraIoContext_t* agora_ctx=agoraSink->agora_ctx;

  /* Retrieve the buffer */
  g_signal_emit_by_name (sink, "pull-sample", &sample);
  if (sample) {

    GstBuffer * in_buffer=gst_sample_get_buffer (sample);

    size_t data_size=gst_buffer_get_size (in_buffer);
    gpointer data=malloc(data_size);
    if(data==NULL){
       g_print("cannot allocate memory!\n");
       return GST_FLOW_ERROR;
    }

    gst_buffer_extract(in_buffer,0, data, data_size);

    GstClockTime in_buffer_pts= GST_BUFFER_CAST(in_buffer)->pts;
    if (agora_ctx==NULL) {
      return GST_FLOW_ERROR;
    }
    
    agoraio_send_audio(agora_ctx, data, data_size,(in_buffer_pts)/1000000);

    free(data);

    gst_sample_unref (sample);
    return GST_FLOW_OK;
  }

  return GST_FLOW_ERROR;
}

int setup_audio_udp(Gstagorasink *agoraSink){

   agoraSink->udpsrc = gst_element_factory_make("udpsrc", "udpsrc");
   if(!agoraSink->udpsrc){
       g_print("failed to create audio udpsrc\n");
   }
   else{
       g_print("created udpsrc successfully\n");
   }

   agoraSink->appAudioSink = gst_element_factory_make("appsink", "appsink");
   if(!agoraSink->appAudioSink){
       g_print("failed to create audio appsink\n");
   }
   else{
       g_print("created appsink successfully\n");
   }

   agoraSink->in_pipeline = gst_pipeline_new ("in-pipeline");
   if(!agoraSink->in_pipeline){
       g_print("failed to create audio pipeline\n");
   }

   //in plugin
   gst_bin_add_many (GST_BIN (agoraSink->in_pipeline),agoraSink->udpsrc, agoraSink->appAudioSink, NULL);
   gst_element_link_many (agoraSink->udpsrc, agoraSink->appAudioSink, NULL);

    g_object_set (G_OBJECT (agoraSink->udpsrc),
             "port", agoraSink->in_port,
              NULL);

    g_object_set (G_OBJECT (agoraSink->udpsrc),
             "auto-multicast", true,
              NULL);

    /*g_object_set (G_OBJECT (agoraSink->udpsrc),
             "multicast-group", "224.1.1.1",
              NULL);*/


    //set the pipeline in playing mode
    gst_element_set_state (agoraSink->in_pipeline, GST_STATE_PLAYING);

    //Configure appsink 
    g_object_set (agoraSink->appAudioSink, "emit-signals", TRUE, NULL);
    g_signal_connect (agoraSink->appAudioSink, "new-sample",
                    G_CALLBACK (new_sample), agoraSink);

    return TRUE;
}


/*this seems not the best way to extract sps/pps info as the src plugin type might differ 
than our use case.  May be we can pass this info to agorasink as input in the future, 
but a dynamic extaction like here is very convenient to the user */
gboolean read_sps_pps_info(Gstagorasink * filter)
{
  GstPad *pad = NULL;
  GstCaps *caps = NULL;

  GstElement * element = gst_bin_get_by_name (GST_BIN(GST_ELEMENT_PARENT(filter)), "rtph264depay0");
  if(element==NULL)
  {
       g_printerr ("cannot find source plugin: rtph264depay0!\n");
       return FALSE;
  }

  pad = gst_element_get_static_pad (element, "sink");
  if (!pad) {
    g_printerr ("Could not retrieve pad sink\n");
    return FALSE;
  }

  caps = gst_pad_get_current_caps (pad);
  if (!caps)
    caps = gst_pad_query_caps (pad, NULL);


  if(caps== NULL)
  {
     return FALSE;
  }

  int j=0, i=0;
  for (i = 0; i < gst_caps_get_size (caps); i++) {

    GstStructure *structure = gst_caps_get_structure (caps, i);

    const char* spspps=gst_structure_get_string(structure, "sprop-parameter-sets");

    memcpy(filter->sps, spspps, SPS_SIZE);
    memcpy(filter->pps, spspps+SPS_SIZE+1, PPS_SIZE);

    g_print("->%s\n", spspps);
    
    gsize ret=0;
    g_base64_decode_inplace((gchar *)filter->sps, &ret);

    g_print("sps data: ");
    for (j=0;j<ret;j++)
    {
      g_print("%x ", filter->sps[j]);
    }
    g_print("\n");

    g_base64_decode_inplace((gchar *)filter->pps, &ret);

    g_print("pps data: ");
    for (j=0;j<ret;j++)
    {
      g_print("%x ", filter->pps[j]);
    }
    g_print("\n");

  }

  //allocate memory
  filter->data_annexb=(uint8_t*)malloc(MAX_AVC_ANNEXB_BUFFER);

  gst_caps_unref (caps);
  gst_object_unref (pad);

  return TRUE;
}

int init_agora(Gstagorasink * filter){

   if (strlen(filter->app_id)==0){
       g_print("app id cannot be empty!\n");
       return -1;
   }

   if (strlen(filter->channel_id)==0){
       g_print("channel id cannot be empty!\n");
       return -1;
   }

   agora_config_t config;

   config.app_id=filter->app_id;               /*appid*/
   config.ch_id=filter->channel_id;            /*channel*/
   config.user_id=filter->user_id;             /*user id*/
   config.is_audiouser=FALSE;                   /*is audio user*/
   config.enc_enable=0;                         /*enable encryption */
   config.enable_dual=0;                        /*enable dual */
   config.dual_vbr=500000;                      /*dual video bitrate*/
   config.dual_width=320;                      /*dual video width*/ 
   config.dual_height=180;                      /*dual video height*/
   config.min_video_jb=12;                     /*initial size of video buffer*/
   config.dfps=30;                             /*dual fps*/
   config.verbose=filter->verbose;             /*log level*/
   config.fn=NULL;                             /*signal function to call*/
   config.userData=(void*)(filter);           /*additional params to the signal function*/ ;
   config.in_audio_delay=0;
   config.in_video_delay=0;
   config.out_audio_delay=0;
   config.out_video_delay=0;
   config.sendOnly= 1;                          /*send only flag*/
   config.enableProxy=FALSE;                    /*enable proxy*/
   config.proxy_timeout= 0;                     /*proxy timeout*/
   config.proxy_ips= "";                        /*proxy ips*/
   config.transcode=filter->transcode;          /*proxy ips*/  
   
   /*initialize agora*/
   filter->agora_ctx=agoraio_init(&config);    

   if(filter->agora_ctx==NULL){

      g_print("agora COULD NOT  be initialized\n");
      return -1;   
   }

   //for agorasink, we need to put it in sendonly mode
   agoraio_set_sendonly_flag(filter->agora_ctx, 1);

   filter->last_audio_buffer_pts=0;

   g_print("agora has been successfuly initialized\n");

   setup_audio_udp(filter);


   //only if avc to annexb conversion is required
   if(filter->convert_avc_to_annexb==TRUE &&
      read_sps_pps_info(filter)==FALSE)
   {
      return -1;
   }
  
   return 0;
}

static void
gst_agorasink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  Gstagorasink *filter = GST_AGORASINK (object);
   
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
    case IN_PORT: 
        filter->in_port=g_value_get_int (value);
        break;
    case ENFORCE_AUDIO_DURAION:
        filter->enforce_audio_duration = g_value_get_boolean (value);
        break;
    case CONVERT_AVC_TO_ANNEX_B:
         filter->convert_avc_to_annexb = g_value_get_boolean (value);
         break;
    case TRANSCODE:
        filter->transcode = g_value_get_boolean (value);
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
    case IN_PORT:
        g_value_set_int (value, filter->in_port);
        break;
    case ENFORCE_AUDIO_DURAION:
        g_value_set_boolean (value, filter->enforce_audio_duration);
        break;
    case CONVERT_AVC_TO_ANNEX_B:
         g_value_set_boolean (value, filter->convert_avc_to_annexb);
         break;
    case TRANSCODE:
        g_value_set_boolean (value, filter->transcode);
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

void print_packet(u_int8_t* data, int size){
  
  int i=0;

  for(i=0;i<size;i++){
    printf(" %d ", data[i]);
  }

  printf("\n================================\n");
    
}

void write_to_file(uint8_t* data, uint32_t len)
{

    FILE *fp=fopen("test.h264", "ab");

    fwrite(data, 1, len, fp);

    fclose(fp);
}
bool is_access_unit_boundary_nal(int nal_unit_type) {

  // Check if this packet marks access unit boundary by checking the
  // packet type.
  if (nal_unit_type == 6 ||  // Supplemental enhancement information
      nal_unit_type == 7 ||  // Picture parameter set
      nal_unit_type == 8 ||  // Sequence parameter set
      nal_unit_type == 9 ||  // Access unit delimiter
      (nal_unit_type >= 14 && nal_unit_type <= 18)) {  // Reserved types

    return true;
  }

  return false;
}

uint16_t write_sps_pps(uint8_t* out_buffer, 
                      uint8_t* sps,
                      uint8_t* pps)
{
  uint16_t written_bytes=0;

  const uint8_t sps_byte_count=24;
  const uint8_t pps_byte_count=4;

  uint8_t nal_start_byte[]={0,0,0,1};

  // uint8_t sps[]={0x67,0x4d,0x00,0x1e,0x9d,0xa8,0x28,0x0f,0x69,0xb8,0x08,0x08,0x0a,0x00,0x00,0x03,0x00,0x02,0x00,0x00,0x03,0x00,0x65,0x08};
  //uint8_t pps[]={0x68,0xee,0x3c,0x80};

  //sps
  memcpy(out_buffer+written_bytes, &nal_start_byte[0], sizeof(nal_start_byte));
  written_bytes +=sizeof(nal_start_byte);

  memcpy(out_buffer+written_bytes, &sps[0], sps_byte_count);
  written_bytes +=sps_byte_count;

  //pps
  memcpy(out_buffer+written_bytes, &nal_start_byte[0], sizeof(nal_start_byte));
  written_bytes +=sizeof(nal_start_byte);

  memcpy(out_buffer+written_bytes, &pps[0], pps_byte_count);
  written_bytes +=pps_byte_count;

  return written_bytes;
}

int  get_nal_size(gpointer data)
{
  int byte0=((uint8_t*)(data))[0];
  int byte1=((uint8_t*)(data))[1];
  int byte2=((uint8_t*)(data))[2];
  int byte3=((uint8_t*)(data))[3];

  int nal_size = (byte0 << 24) + (byte1 << 16) + (byte2 << 8) + byte3;

  return nal_size;
}
void dump_bytes(uint8_t* data)
{
  int i;
  for(i=0;i<10;i++)
  {
    g_print("%x ", data[i]);
  }
  g_print("\n");
}

uint16_t convert_avc_to_annexb(gpointer input, 
                               uint16_t input_size, 
                               uint8_t* output,
                               uint8_t* sps,
                               uint8_t* pps)
{
    const uint8_t nal_type_AUD=9;
    uint16_t nal_size=0;

    //this is not true in general, but ok in our case
    //alternatves are 1, 2 or 4 bytes
    const uint8_t size_bytes_width=4;

    uint16_t left_input_bytes=input_size;

    uint8_t* input_scaner=((uint8_t*)(input));
    uint8_t* output_scaner=((uint8_t*)(output));

    //dump_bytes(input_scaner);

    uint8_t nal_start_byte_count=3;

    //we will change this later to be either 001 or 0001
    uint8_t nal_start_byte[]={0,0,1,1};

    uint8_t units_count=0;

    gboolean is_aud_sent = FALSE;

    //one avc nal can contain multiple units
    while(left_input_bytes>0)
    {
        //dump_bytes(input_scaner);
        nal_size=get_nal_size(input_scaner);

        uint8_t  fmt=((uint8_t*)(input_scaner))[size_bytes_width];

        int nal_unit_type = fmt & 0x1F;

        input_scaner +=size_bytes_width;
        left_input_bytes -=size_bytes_width;

        //need to append AUD?
        if(is_aud_sent==FALSE &&
          (nal_unit_type==1 || nal_unit_type==5 || nal_unit_type==6))
        {
          static u_char   aud_nal[] = {0x00, 0x00, 0x00, 0x01, 0x09, 0xf0};
          memcpy(output_scaner, aud_nal, sizeof(aud_nal));

          output_scaner +=sizeof(aud_nal);

          is_aud_sent=TRUE;

          //g_print("AUD nal added, nal type: %d\n",nal_unit_type );
        }

        //write sps/pps if required
        if(units_count>1 || nal_unit_type !=nal_type_AUD)
        {
           uint16_t written_sps_pps_bytes=write_sps_pps(output_scaner, sps, pps);
           output_scaner +=written_sps_pps_bytes;
        }

        //in case of unit boundary, we will need to add 0001 (four bytes instead of 3)
        if(is_access_unit_boundary_nal(nal_unit_type)==true)
        {
          nal_start_byte[2]=0;
          nal_start_byte_count=4;
        }

        //validate remaining input bytes
        if(left_input_bytes<(nal_size))
        {
           g_print("Agorasink: no enough bytes remaining in the input packet\n");
           return 0;
        }

        memcpy(output_scaner, nal_start_byte, nal_start_byte_count);
        output_scaner +=nal_start_byte_count;

        //write nalu data
        memcpy(output_scaner, input_scaner, nal_size);

        output_scaner +=nal_size;
        input_scaner +=nal_size;
        left_input_bytes -=nal_size;
        
        units_count++;
    }

    return (uint16_t)(output_scaner-output);
}


/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_agorasink_chain (GstPad * pad, GstObject * parent, GstBuffer * in_buffer)
{
  Gstagorasink *filter;
  filter = GST_AGORASINK (parent);

  size_t      data_size=0;
  gboolean    is_key_frame=0;


  //TODO: we need a better position to initialize agora. 
  //gst_agorasink_init() is good, however, it is called before reading app and channels ids
  if(filter->agora_ctx==NULL && init_agora(filter)!=0){
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
      is_key_frame=TRUE;
      if(filter->is_key_frame_seen==FALSE)
      {
         filter->is_key_frame_seen=TRUE;

         g_print("*********first key frame sent **********\n");
      }
  }


   GstClock* clock = gst_element_get_clock (GST_ELEMENT_CAST (filter));
   GstClockTime in_buffer_pts= gst_clock_get_time (clock); //GST_BUFFER_CAST(in_buffer)->pts;
   in_buffer_pts /=1000000;


  if(filter->audio==FALSE){

    if(filter->convert_avc_to_annexb==TRUE)
    {
         if(filter->is_key_frame_seen)
         {
            //this function does the actual conversion
            uint16_t current_data_annexb_size=
             convert_avc_to_annexb(data, data_size, filter->data_annexb,filter->sps, filter->pps);

            agoraio_send_video(filter->agora_ctx, filter->data_annexb, current_data_annexb_size,is_key_frame, in_buffer_pts);
             
            //write_to_file(filter->data_annexb, current_data_annexb_size);
         }
    }
    else
    {
      agoraio_send_video(filter->agora_ctx, data, data_size,is_key_frame, in_buffer_pts);
    }
  }
  else{
      long duration=(long)(GST_BUFFER_DURATION (in_buffer)/1000000.0);

      if(filter->enforce_audio_duration){
         // g_print("duration: %ld\n", duration);
          agoraio_send_audio_with_duration(filter->agora_ctx, data, data_size, in_buffer_pts, duration);
          filter->last_audio_buffer_pts=in_buffer_pts;
      }
      else{
          agoraio_send_audio(filter->agora_ctx, data, data_size, in_buffer_pts);
      }
  }

    
  if (filter->verbose == true){
      g_print ("agorasink: received %" G_GSIZE_FORMAT" bytes!\n",data_size);
      print_packet(data, 10);
      g_print("agorasink: is key frame: %d\n", is_key_frame);
  }

  free(data);
  //free(data_annexb); 
  filter->ts+=30;

  gst_buffer_unref (in_buffer);

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
#define PACKAGE "agorasink"
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
